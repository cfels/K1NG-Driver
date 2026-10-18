#include "include.h"
#include "data.h"
#include "driver.h"
#include <assert.h>

struct libusb_device_handle {
    int unused;
};
static struct libusb_device_handle device;
static unsigned char request[DPI_REPORT_SIZE];
static unsigned char slots[4][DPI_MAX_STAGES][DPI_STAGE_SIZE];
static unsigned char active_stage = 1;
static unsigned char profile;
static unsigned char stage_count = 6;
static int initialized;
static int wireless;
static int reads;
static int transactions;
static int writes;
static int config_reads;
static int online_queries;
static int profile_queries;
static int claims;
static int releases;
static int closes;
static unsigned char last_address;
static int stale_reports;
static int battery_queries;
static unsigned char battery_percentage = 65;
static unsigned char battery_charging;
static struct libusb_transfer *pending_transfer;
static int pending_cancelled;
static int allocations;
static int frees;
static int early_replies;
static int event_error_returned;

static void service_pending(void);

enum {
    NORMAL,
    LEGACY_READ_REPLY,
    STATUS_THEN_REPLY,
    SHORT_WRITE,
    WRITE_ERROR,
    NO_ACK,
    WRONG_ACK,
    SHORT_ACK,
    ACK_WITHOUT_APPLY,
    READ_TIMEOUT,
    READ_ECHO,
    BAD_OUTER_CHECKSUM,
    BAD_PAIR,
    ZERO_COUNT,
    TOO_MANY_STAGES,
    BAD_STAGE,
    BAD_READ_LENGTH,
    WRONG_READ_ADDRESS,
    BAD_PROFILE,
    PROFILE_UNSUPPORTED,
    BAD_DPI_WORD,
    UNSUPPORTED_DPI_WORD,
    STAGE_BEFORE,
    PROFILE_BEFORE,
    STAGE_AFTER,
    PROFILE_AFTER,
    READBACK_TIMEOUT,
    CLAIM_ERROR,
    DETACH_ERROR,
    RELEASE_ERROR,
    NO_DEVICE,
    SHORT_READ_REQUEST,
    PROFILE_ECHO,
    BAD_STATUS,
    OFFLINE,
    OFFLINE_BUT_RESPONSIVE,
    INVALID_ONLINE,
    ALLOC_ERROR,
    SUBMIT_ERROR,
    EARLY_REPLY,
    EVENT_ERROR,
    ONLINE_TIMEOUT_ONCE,
    CONFIG_TIMEOUT_ONCE,
    CONFIG_TIMEOUT_TWICE,
    PROFILE_TIMEOUT_ONCE,
    QUEUED_STATE,
    BUSY_QUEUE,
    BATTERY_BAD_PERCENTAGE,
    BATTERY_BAD_FLAG,
    BATTERY_BAD_LENGTH,
    BATTERY_BAD_CHECKSUM,
    BATTERY_SHORT_TRANSFER,
    BATTERY_SHORT_REPLY,
    BATTERY_ECHO,
    BATTERY_TIMEOUT
};
static int scenario;

static void checksum(unsigned char *data) {
    unsigned int sum = 0;
    for (int i = 0; i < DPI_REPORT_SIZE - 1; i++)
        sum += data[i];
    data[16] = (unsigned char)(0x55 - sum);
}

static void reset(void) {
    assert(pending_transfer == NULL);
    const unsigned char dpi800[] = {0x0f, 0x0f, 0x00, 0x37};
    for (int p = 0; p < 4; p++) {
        for (int s = 0; s < DPI_MAX_STAGES; s++)
            memcpy(slots[p][s], dpi800, DPI_STAGE_SIZE);
    }
    reads = transactions = writes = config_reads = claims = releases = closes = 0;
    stale_reports = 0;
    battery_queries = 0;
    online_queries = profile_queries = 0;
    allocations = frees = early_replies = event_error_returned = 0;
    initialized = 1;
}

int libusb_init(libusb_context **ctx) {
    assert(ctx == NULL);
    if (!initialized) {
        const char *fail = getenv("K1NG_TEST_ACK_WITHOUT_APPLY");
        if (fail && strcmp(fail, "1") == 0)
            scenario = ACK_WITHOUT_APPLY;
        const char *online_zero = getenv("K1NG_TEST_ONLINE_ZERO");
        if (online_zero && strcmp(online_zero, "1") == 0)
            scenario = OFFLINE_BUT_RESPONSIVE;
        const char *early_reply = getenv("K1NG_TEST_EARLY_REPLY");
        if (early_reply && strcmp(early_reply, "1") == 0)
            scenario = EARLY_REPLY;
        const char *wake = getenv("K1NG_TEST_WAKE_READS");
        if (wake && strcmp(wake, "1") == 0)
            scenario = CONFIG_TIMEOUT_TWICE;
        const char *wireless_mode = getenv("K1NG_TEST_WIRELESS");
        if (wireless_mode && strcmp(wireless_mode, "1") == 0)
            wireless = 1;
        const char *no_device = getenv("K1NG_TEST_NO_DEVICE");
        if (no_device && strcmp(no_device, "1") == 0)
            scenario = NO_DEVICE;
        const char *charging_mode = getenv("K1NG_TEST_CHARGING");
        if (charging_mode && strcmp(charging_mode, "1") == 0) {
            battery_charging = 1;
            battery_percentage = 90;
        }
        reset();
    }
    return 0;
}
void libusb_exit(libusb_context *ctx) { assert(ctx == NULL); }

libusb_device_handle *libusb_open_device_with_vid_pid(libusb_context *ctx, uint16_t vid,
                                                      uint16_t pid) {
    assert(ctx == NULL && vid == TARGET_VID);
    assert(pid == TARGET_PID_WIRED || pid == TARGET_PID_WIRELESS);
    if (scenario == NO_DEVICE || (wireless && pid == TARGET_PID_WIRED))
        return NULL;
    return &device;
}
int libusb_set_auto_detach_kernel_driver(libusb_device_handle *handle, int enable) {
    assert(handle == &device && enable == 1);
    return scenario == DETACH_ERROR ? LIBUSB_ERROR_OTHER : 0;
}
int libusb_claim_interface(libusb_device_handle *handle, int interface_number) {
    assert(handle == &device && interface_number == 1);
    claims++;
    return scenario == CLAIM_ERROR ? LIBUSB_ERROR_BUSY : 0;
}
int libusb_release_interface(libusb_device_handle *handle, int interface_number) {
    assert(handle == &device && interface_number == 1);
    releases++;
    return scenario == RELEASE_ERROR ? LIBUSB_ERROR_OTHER : 0;
}
void libusb_close(libusb_device_handle *handle) {
    assert(handle == &device);
    assert(pending_transfer == NULL && allocations == frees);
    closes++;
}
const char *libusb_error_name(int code) {
    assert(code < 0);
    return "mock USB error";
}

static int control_transfer(libusb_device_handle *handle, uint8_t request_type, uint8_t code,
                            uint16_t value, uint16_t index, unsigned char *data, uint16_t length,
                            unsigned int timeout) {
    assert(handle == &device && request_type == 0x21 && code == 0x09);
    assert(value == 0x0208 && index == 1 && length == DPI_REPORT_SIZE);
    assert(timeout == ((data[1] == 7 || data[1] == 4) ? 4000 : 1000));
    assert(data[0] == 8 && data[2] == 0 && data[3] == 0);
    memcpy(request, data, length);
    reads = 0;
    transactions++;
    if (data[1] == 3) {
        assert(data[4] == 0 && data[5] == 0);
        online_queries++;
        return length;
    }
    if (data[1] == 4) {
        assert(data[4] == 0 && data[5] == 0);
        battery_queries++;
        return scenario == BATTERY_SHORT_TRANSFER ? length - 1 : length;
    }
    if (data[1] == 0x08 && data[4] == 0) {
        assert(data[5] == DPI_CONFIG_SIZE);
        config_reads++;
        if ((scenario == STAGE_BEFORE && config_reads == 2) ||
            (scenario == STAGE_AFTER && config_reads == 3))
            active_stage ^= 1;
        if ((scenario == PROFILE_BEFORE && config_reads == 2) ||
            (scenario == PROFILE_AFTER && config_reads == 3))
            profile ^= 1;
        return scenario == SHORT_READ_REQUEST ? length - 1 : length;
    }
    if (data[1] == 0x0e) {
        assert(data[4] == 0 && data[5] == 0);
        profile_queries++;
        return length;
    }
    assert((data[1] == 0x07 || data[1] == 0x08) && data[5] == DPI_STAGE_SIZE);
    assert(data[4] >= DPI_ADDRESS_BASE &&
           data[4] < DPI_ADDRESS_BASE + DPI_MAX_STAGES * DPI_STAGE_SIZE);
    assert((data[4] - DPI_ADDRESS_BASE) % DPI_STAGE_SIZE == 0);
    if (data[1] == 0x08)
        return length;
    writes++;
    last_address = data[4];
    if (scenario == WRITE_ERROR)
        return LIBUSB_ERROR_PIPE;
    if (scenario == SHORT_WRITE)
        return length - 1;
    if (scenario != ACK_WITHOUT_APPLY) {
        memcpy(slots[profile][(data[4] - DPI_ADDRESS_BASE) / DPI_STAGE_SIZE], data + 6,
               DPI_STAGE_SIZE);
    }
    return length;
}

int libusb_control_transfer(libusb_device_handle *handle, uint8_t request_type, uint8_t code,
                            uint16_t value, uint16_t index, unsigned char *data, uint16_t length,
                            unsigned int timeout) {
    assert(pending_transfer != NULL && !pending_cancelled);
    int r = control_transfer(handle, request_type, code, value, index, data, length, timeout);
    if (r == length && scenario == EARLY_REPLY) {
        service_pending();
        early_replies++;
    }
    return r;
}

struct libusb_transfer *libusb_alloc_transfer(int iso_packets) {
    assert(iso_packets == 0);
    if (scenario == ALLOC_ERROR)
        return NULL;
    struct libusb_transfer *transfer = calloc(1, sizeof(*transfer));
    assert(transfer != NULL);
    allocations++;
    return transfer;
}

void libusb_free_transfer(struct libusb_transfer *transfer) {
    assert(transfer != NULL && pending_transfer != transfer);
    frees++;
    free(transfer);
}

int libusb_submit_transfer(struct libusb_transfer *transfer) {
    assert(pending_transfer == NULL);
    assert(transfer->dev_handle == &device && transfer->endpoint == 0x82);
    assert(transfer->type == LIBUSB_TRANSFER_TYPE_INTERRUPT && transfer->length == DPI_REPORT_SIZE);
    assert(transfer->timeout > 0 && transfer->timeout <= 4000);
    if (scenario == SUBMIT_ERROR)
        return LIBUSB_ERROR_IO;
    pending_transfer = transfer;
    pending_cancelled = 0;
    return 0;
}

int libusb_cancel_transfer(struct libusb_transfer *transfer) {
    assert(transfer == pending_transfer);
    pending_cancelled = 1;
    return 0;
}

static void service_pending(void) {
    struct libusb_transfer *transfer = pending_transfer;
    assert(transfer != NULL);
    pending_transfer = NULL;
    transfer->actual_length = 0;
    if (pending_cancelled) {
        transfer->status = LIBUSB_TRANSFER_CANCELLED;
    } else {
        int r = libusb_interrupt_transfer(transfer->dev_handle, transfer->endpoint,
                                          transfer->buffer, transfer->length,
                                          &transfer->actual_length, transfer->timeout);
        transfer->status = r == 0 ? LIBUSB_TRANSFER_COMPLETED : LIBUSB_TRANSFER_TIMED_OUT;
    }
    transfer->callback(transfer);
}

int libusb_handle_events_completed(libusb_context *ctx, int *completed) {
    assert(ctx == NULL && completed != NULL);
    if (*completed)
        return 0;
    if (scenario == EVENT_ERROR && !event_error_returned) {
        event_error_returned = 1;
        return LIBUSB_ERROR_IO;
    }
    service_pending();
    return 0;
}

int libusb_interrupt_transfer(libusb_device_handle *handle, unsigned char endpoint,
                              unsigned char *data, int length, int *transferred,
                              unsigned int timeout) {
    assert(handle == &device && endpoint == 0x82 && length == DPI_REPORT_SIZE);
    assert(timeout > 0 && timeout <= 4000);
    if (stale_reports || (scenario == BUSY_QUEUE && timeout == 1)) {
        const unsigned char stale[] = {8, 8, 0, 0, 0, 6, 1, 0x54, 6, 0x4f, 0, 0x55, 0, 0, 0, 0, 0};
        memcpy(data, stale, sizeof(stale));
        checksum(data);
        *transferred = length;
        stale_reports = 0;
        return 0;
    }
    if (timeout == 1)
        return LIBUSB_ERROR_TIMEOUT;
    reads++;
    if ((request[1] == 4 &&
         (scenario == BATTERY_TIMEOUT || (scenario == BATTERY_SHORT_REPLY && reads > 1))) ||
        (scenario == READ_TIMEOUT && request[1] == 8) ||
        (scenario == ONLINE_TIMEOUT_ONCE && request[1] == 3 && online_queries == 1) ||
        (scenario == CONFIG_TIMEOUT_ONCE && request[1] == 8 && request[4] == 0 &&
         config_reads == 1) ||
        (scenario == CONFIG_TIMEOUT_TWICE && request[1] == 8 && request[4] == 0 &&
         config_reads <= 2) ||
        (scenario == PROFILE_TIMEOUT_ONCE && request[1] == 0x0e && profile_queries == 1) ||
        (scenario == OFFLINE && (request[1] == 8 || request[1] == 0x0e)) ||
        (scenario == PROFILE_UNSUPPORTED && request[1] == 0x0e) ||
        (scenario == NO_ACK && request[1] == 7) ||
        (scenario == READBACK_TIMEOUT && request[1] == 8 && writes) ||
        ((scenario == WRONG_ACK || scenario == SHORT_ACK || scenario == WRONG_READ_ADDRESS) &&
         reads > 1)) {
        return LIBUSB_ERROR_TIMEOUT;
    }
    memcpy(data, request, length);
    *transferred = length;
    if (scenario == STATUS_THEN_REPLY && reads == 1) {
        data[1] = 0x0a;
        checksum(data);
        return 0;
    }
    if (request[1] == 3) {
        data[5] = 1;
        data[6] = scenario == INVALID_ONLINE ? 2 : 1;
        if (scenario == OFFLINE || scenario == OFFLINE_BUT_RESPONSIVE) {
            const unsigned char offline[] = {0x08, 0x03, 0, 0, 0, 1, 0, 0xac, 0x19,
                                             0x66, 0,    0, 0, 0, 0, 0, 0x1e};
            memcpy(data, offline, sizeof(offline));
        }
        if (scenario == QUEUED_STATE)
            stale_reports = 1;
    } else if (request[1] == 4) {
        if (scenario == BATTERY_ECHO)
            return 0;
        data[5] = scenario == BATTERY_BAD_LENGTH ? 1 : 2;
        data[6] = scenario == BATTERY_BAD_PERCENTAGE ? 101 : battery_percentage;
        data[7] = scenario == BATTERY_BAD_FLAG ? 2 : battery_charging;
        data[8] = 0x0f;
        data[9] = 0x83;
        if (scenario == BATTERY_SHORT_REPLY)
            *transferred = length - 1;
    } else if (request[1] == 8) {
        if (scenario == READ_ECHO)
            return 0;
        if (request[4] == 0) {
            const unsigned char config[] = {1,
                                            0x54,
                                            stage_count,
                                            (unsigned char)(0x55 - stage_count),
                                            active_stage,
                                            (unsigned char)(0x55 - active_stage)};
            memcpy(data + 6, config, sizeof(config));
            if (scenario == BAD_PAIR)
                data[9] ^= 1;
            if (scenario == ZERO_COUNT || scenario == TOO_MANY_STAGES) {
                data[8] = scenario == ZERO_COUNT ? 0 : 9;
                data[9] = (unsigned char)(0x55 - data[8]);
            }
            if (scenario == BAD_STAGE) {
                data[10] = stage_count;
                data[11] = (unsigned char)(0x55 - stage_count);
            }
        } else {
            memcpy(data + 6, slots[profile][(request[4] - DPI_ADDRESS_BASE) / DPI_STAGE_SIZE],
                   DPI_STAGE_SIZE);
            if (scenario == BAD_DPI_WORD)
                data[9] ^= 1;
            if (scenario == UNSUPPORTED_DPI_WORD) {
                data[8] = 0x11;
                data[9] -= 0x11;
            }
        }
        if (scenario == LEGACY_READ_REPLY)
            data[1] = 7;
        if (scenario == BAD_READ_LENGTH)
            data[5]--;
        if (scenario == WRONG_READ_ADDRESS)
            data[4] += 4;
        if (scenario == BAD_STATUS)
            data[2] = 1;
    } else if (request[1] == 0x0e) {
        if (scenario == PROFILE_ECHO)
            return 0;
        data[5] = 1;
        data[6] = scenario == BAD_PROFILE ? 4 : profile;
    } else if (request[1] == 7) {
        if (scenario == WRONG_ACK)
            data[6] ^= 1;
        if (scenario == SHORT_ACK)
            *transferred = length - 1;
    }
    checksum(data);
    if (scenario == BAD_OUTER_CHECKSUM && request[1] == 8)
        data[16] ^= 1;
    if (scenario == BATTERY_BAD_CHECKSUM && request[1] == 4)
        data[16] ^= 1;
    return 0;
}

#ifndef TEST_USB_LIBRARY
int main(void) {
    for (wireless = 0; wireless <= 1; wireless++) {
        for (profile = 0; profile < 4; profile++) {
            for (active_stage = 0; active_stage < DPI_MAX_STAGES; active_stage++) {
                scenario = NORMAL;
                stage_count = DPI_MAX_STAGES;
                reset();
                unsigned char old_slots[sizeof(slots)];
                memcpy(old_slots, slots, sizeof(slots));
                int dpi = active_stage == 0 ? 50 : 26000;
                assert(run_driver(dpi) == 0);
                assert(writes == 1 &&
                       last_address == DPI_ADDRESS_BASE + active_stage * DPI_STAGE_SIZE);
                unsigned char expected[DPI_REPORT_SIZE];
                assert(get_payload(dpi, last_address, expected) == 0);
                assert(memcmp(slots[profile][active_stage], expected + 6, DPI_STAGE_SIZE) == 0);
                memcpy(slots[profile][active_stage], (unsigned char[]){0x0f, 0x0f, 0, 0x37},
                       DPI_STAGE_SIZE);
                assert(memcmp(old_slots, slots, sizeof(slots)) == 0);
            }
        }
        for (scenario = NORMAL; scenario <= BUSY_QUEUE; scenario++) {
            active_stage = 1;
            profile = 0;
            stage_count = 6;
            reset();
            int success = scenario == NORMAL || scenario == LEGACY_READ_REPLY ||
                          scenario == STATUS_THEN_REPLY || scenario == OFFLINE_BUT_RESPONSIVE ||
                          scenario == EARLY_REPLY || scenario == ONLINE_TIMEOUT_ONCE ||
                          scenario == CONFIG_TIMEOUT_ONCE || scenario == CONFIG_TIMEOUT_TWICE ||
                          scenario == PROFILE_TIMEOUT_ONCE || scenario == QUEUED_STATE;
            assert((run_driver(400) == 0) == success);
            int attempted_write = scenario <= ACK_WITHOUT_APPLY || scenario == STAGE_AFTER ||
                                  scenario == PROFILE_AFTER || scenario == READBACK_TIMEOUT ||
                                  scenario == RELEASE_ERROR || scenario == OFFLINE_BUT_RESPONSIVE ||
                                  scenario == EARLY_REPLY || scenario == ONLINE_TIMEOUT_ONCE ||
                                  scenario == CONFIG_TIMEOUT_ONCE ||
                                  scenario == CONFIG_TIMEOUT_TWICE ||
                                  scenario == PROFILE_TIMEOUT_ONCE || scenario == QUEUED_STATE;
            assert(writes == attempted_write);
            if (scenario == OFFLINE || scenario == READ_TIMEOUT)
                assert(config_reads == 3 && writes == 0);
            if (scenario == ONLINE_TIMEOUT_ONCE)
                assert(online_queries == 2 && writes == 1);
            if (scenario == CONFIG_TIMEOUT_ONCE)
                assert(config_reads == 4 && writes == 1);
            if (scenario == CONFIG_TIMEOUT_TWICE)
                assert(config_reads == 5 && writes == 1);
            if (scenario == PROFILE_TIMEOUT_ONCE)
                assert(profile_queries == 4 && writes == 1);
            if (scenario == INVALID_ONLINE)
                assert(config_reads == 0 && writes == 0);
            if (scenario == EARLY_REPLY)
                assert(early_replies == transactions && writes == 1);
            assert(releases ==
                   (scenario != NO_DEVICE && scenario != DETACH_ERROR && scenario != CLAIM_ERROR));
            assert(closes == (scenario != NO_DEVICE));
        }
    }
    scenario = NORMAL;
    profile = 0;
    active_stage = 1;
    stage_count = 6;
    reset();
    assert(run_driver_at_address(400, DPI_ADDRESS_BASE) == 0);
    assert(last_address == DPI_ADDRESS_BASE);
    assert(memcmp(slots[0][1], (unsigned char[]){0x0f, 0x0f, 0, 0x37}, DPI_STAGE_SIZE) == 0);
    reset();
    assert(run_driver_at_address(400, 0x28) != 0 && writes == 0);
    reset();
    assert(run_driver(51) != 0 && transactions == 0);
    assert(run_driver_at_address(100, 0xff) != 0 && transactions == 0);
    for (wireless = 0; wireless <= 1; wireless++) {
        if (wireless) {
            scenario = NORMAL;
            reset();
            assert(run_battery() != 0);
            assert(transactions == 0 && battery_queries == 0 && writes == 0 && config_reads == 0);
            assert(claims == 0 && releases == 0 && closes == 0);
            continue;
        }
        for (battery_charging = 0; battery_charging <= 1; battery_charging++) {
            for (int percentage = 0; percentage <= 100; percentage += 50) {
                battery_percentage = (unsigned char)percentage;
                scenario = NORMAL;
                reset();
                assert(run_battery() == 0);
                assert(writes == 0 && config_reads == 0 && battery_queries == 1 &&
                       transactions == 2);
                assert(releases == 1 && closes == 1);
            }
        }
        for (scenario = BATTERY_BAD_PERCENTAGE; scenario <= BATTERY_TIMEOUT; scenario++) {
            battery_percentage = 65;
            battery_charging = 0;
            reset();
            assert(run_battery() != 0);
            assert(writes == 0 && config_reads == 0 && battery_queries == 1);
            assert(releases == 1 && closes == 1);
        }
        scenario = OFFLINE;
        reset();
        assert(run_battery() != 0 && writes == 0 && battery_queries == 0);
        scenario = OFFLINE_BUT_RESPONSIVE;
        reset();
        assert(run_battery() != 0 && writes == 0 && battery_queries == 0 && config_reads == 0);
    }
    scenario = NO_DEVICE;
    reset();
    assert(run_battery() != 0 && transactions == 0);
    assert(claims == 0 && releases == 0 && closes == 0);
    return 0;
}
#endif
