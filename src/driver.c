#include "../includes/include.h"
#include "driver.h"
#include "data.h"
#include <stdint.h>
#include <time.h>

struct mouse_state {
    unsigned char config[DPI_CONFIG_SIZE];
    unsigned char profile;
    unsigned char address;
};

static void print_bytes(const char *label, const unsigned char *data, size_t length) {
    fprintf(stderr, "%s:", label);
    for (size_t i = 0; i < length; i++) {
        fprintf(stderr, " %02x", data[i]);
    }
    fputc('\n', stderr);
}

static void LIBUSB_CALL interrupt_complete(struct libusb_transfer *transfer) {
    int *completed = transfer->user_data;
    *completed = 1;
}

static void cancel_interrupt(struct libusb_transfer *transfer) {
    int r = libusb_cancel_transfer(transfer);
    if (r < 0 && r != LIBUSB_ERROR_NOT_FOUND) {
        fprintf(stderr, "cannot cancel response transfer: %s\n", libusb_error_name(r));
    }
}

static int complete_interrupt(struct libusb_transfer *transfer, int *completed) {
    int error = 0;
    while (!*completed) {
        int r = libusb_handle_events_completed(NULL, completed);
        if (r < 0 && r != LIBUSB_ERROR_INTERRUPTED && error == 0) {
            error = r;
            cancel_interrupt(transfer);
        }
    }
    if (error != 0) {
        return error;
    }
    switch (transfer->status) {
    case LIBUSB_TRANSFER_COMPLETED:
        return 0;
    case LIBUSB_TRANSFER_TIMED_OUT:
        return LIBUSB_ERROR_TIMEOUT;
    case LIBUSB_TRANSFER_STALL:
        return LIBUSB_ERROR_PIPE;
    case LIBUSB_TRANSFER_NO_DEVICE:
        return LIBUSB_ERROR_NO_DEVICE;
    case LIBUSB_TRANSFER_OVERFLOW:
        return LIBUSB_ERROR_OVERFLOW;
    default:
        return LIBUSB_ERROR_IO;
    }
}

static int wait_for_reply(const unsigned char *request, unsigned char *response,
                          struct libusb_transfer *transfer, int *completed, unsigned int timeout_ms,
                          bool report_timeout) {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("cannot start response timeout");
        return -1;
    }

    unsigned int timeout = timeout_ms;
    while (timeout > 0) {
        int r = complete_interrupt(transfer, completed);
        if (r < 0) {
            if (r != LIBUSB_ERROR_TIMEOUT || report_timeout) {
                fprintf(stderr, "response failed for command 0x%02x at 0x%02x%02x: %s\n",
                        request[1], request[3], request[4], libusb_error_name(r));
                print_bytes("request", request, DPI_REPORT_SIZE);
            }
            return r;
        }
        if (transfer->actual_length == DPI_REPORT_SIZE && response[0] == 0x08 &&
            (response[1] == request[1] || (request[1] == 0x08 && response[1] == 0x07)) &&
            response[3] == request[3] && response[4] == request[4]) {
            unsigned int sum = 0;
            for (int i = 0; i < DPI_REPORT_SIZE; i++) {
                sum += response[i];
            }
            if ((sum & 0xff) != 0x55 || response[2] != 0 ||
                (request[1] == 0x08 && response[5] != request[5]) ||
                (request[1] == 0x0e && (response[5] < 1 || response[5] > 10))) {
                print_bytes("invalid response", response, DPI_REPORT_SIZE);
                return -1;
            }
            if (request[1] != 0x07 || memcmp(response, request, DPI_REPORT_SIZE) == 0) {
                return 0;
            }
        }

        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            perror("cannot check response timeout");
            return -1;
        }
        int64_t elapsed =
            (int64_t)(now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
        timeout = elapsed >= timeout_ms ? 0 : (unsigned int)(timeout_ms - elapsed);
        if (timeout > 0) {
            transfer->timeout = timeout;
            r = libusb_submit_transfer(transfer);
            if (r < 0) {
                fprintf(stderr, "cannot queue response transfer: %s\n", libusb_error_name(r));
                return r;
            }
            *completed = 0;
        }
    }

    if (report_timeout) {
        fprintf(stderr, "timed out waiting for matching device response\n");
        print_bytes("request", request, DPI_REPORT_SIZE);
    }
    return LIBUSB_ERROR_TIMEOUT;
}

static int discard_pending_replies(libusb_device_handle *handle) {
    for (int pending = 0; pending < 64; pending++) {
        unsigned char response[DPI_REPORT_SIZE];
        int received = 0;
        int r = libusb_interrupt_transfer(handle, 0x82, response, sizeof(response), &received, 1);
        if (r == LIBUSB_ERROR_TIMEOUT) {
            return 0;
        }
        if (r < 0) {
            fprintf(stderr, "cannot clear pending responses: %s\n", libusb_error_name(r));
            return -1;
        }
    }
    fprintf(stderr, "device response queue remains busy\n");
    return -1;
}

static int exchange_once(libusb_device_handle *handle, unsigned char *request,
                         unsigned char *response, unsigned int timeout_ms, bool report_timeout) {
    if (discard_pending_replies(handle) != 0) {
        return -1;
    }
    struct libusb_transfer *transfer = libusb_alloc_transfer(0);
    if (!transfer) {
        fprintf(stderr, "cannot allocate response transfer\n");
        return -1;
    }
    int completed = 0;
    libusb_fill_interrupt_transfer(transfer, handle, 0x82, response, DPI_REPORT_SIZE,
                                   interrupt_complete, &completed, timeout_ms);
    int r = libusb_submit_transfer(transfer);
    if (r < 0) {
        fprintf(stderr, "cannot queue response transfer: %s\n", libusb_error_name(r));
        libusb_free_transfer(transfer);
        return r;
    }
    int status = -1;
    int transferred = libusb_control_transfer(handle, 0x21, 0x09, 0x0208, 0x0001, request,
                                              DPI_REPORT_SIZE, timeout_ms);
    if (transferred < 0) {
        status = transferred;
        if (status != LIBUSB_ERROR_TIMEOUT || report_timeout) {
            fprintf(stderr, "report transfer failed: %s\n", libusb_error_name(transferred));
        }
    } else if (transferred != DPI_REPORT_SIZE) {
        fprintf(stderr, "incomplete report: %d of %d bytes written\n", transferred,
                DPI_REPORT_SIZE);
    } else {
        status =
            wait_for_reply(request, response, transfer, &completed, timeout_ms, report_timeout);
    }
    if (!completed) {
        cancel_interrupt(transfer);
        r = complete_interrupt(transfer, &completed);
        if (r != LIBUSB_ERROR_IO && r != 0) {
            fprintf(stderr, "response cleanup failed: %s\n", libusb_error_name(r));
        }
    }
    libusb_free_transfer(transfer);
    return status;
}

static int exchange(libusb_device_handle *handle, unsigned char *request, unsigned char *response) {
    bool read = request[1] == 0x03 || request[1] == 0x08 || request[1] == 0x0e;
    int attempts = read ? 3 : 1;
    for (int attempt = 0; attempt < attempts; attempt++) {
        int r =
            exchange_once(handle, request, response, read ? 1000 : 4000, attempt == attempts - 1);
        if (r != LIBUSB_ERROR_TIMEOUT || attempt == attempts - 1) {
            if (read && r == LIBUSB_ERROR_TIMEOUT) {
                fprintf(stderr, "mouse did not respond; move or click it if asleep, then retry\n");
            }
            return r;
        }
        if (attempt == 0) {
            fprintf(stderr, "waiting for mouse response; retrying read automatically\n");
        }
    }
    return LIBUSB_ERROR_TIMEOUT;
}

static int read_register(libusb_device_handle *handle, unsigned char address, unsigned char length,
                         unsigned char *data) {
    unsigned char request[DPI_REPORT_SIZE];
    unsigned char response[DPI_REPORT_SIZE];
    if (get_read_payload(address, length, request) != 0 ||
        exchange(handle, request, response) != 0) {
        return -1;
    }
    memcpy(data, response + 6, length);
    return 0;
}

static int read_mouse_state(libusb_device_handle *handle, struct mouse_state *state) {
    if (read_register(handle, 0x00, DPI_CONFIG_SIZE, state->config) != 0) {
        fprintf(stderr, "cannot read active DPI stage; no fixed-target fallback\n");
        return -1;
    }
    if (resolve_dpi_target(state->config, &state->address) != 0) {
        print_bytes("invalid DPI configuration", state->config, DPI_CONFIG_SIZE);
        return -1;
    }
    unsigned char request[DPI_REPORT_SIZE];
    unsigned char response[DPI_REPORT_SIZE];
    get_profile_payload(request);
    if (exchange(handle, request, response) != 0) {
        fprintf(stderr, "cannot read active profile\n");
        return -1;
    }
    if (response[6] > 3) {
        print_bytes("unsupported profile response", response, DPI_REPORT_SIZE);
        return -1;
    }
    state->profile = response[6];
    return 0;
}

static int same_state(const struct mouse_state *a, const struct mouse_state *b) {
    return a->profile == b->profile && a->config[2] == b->config[2] && a->config[4] == b->config[4];
}

static int check_online(libusb_device_handle *handle, bool require_online) {
    unsigned char online_request[DPI_REPORT_SIZE] = {[0] = 0x08, [1] = 0x03, [16] = 0x4a};
    unsigned char online_response[DPI_REPORT_SIZE];
    if (exchange(handle, online_request, online_response) != 0) {
        return -1;
    }
    if (online_response[5] != 1 || online_response[6] > 1) {
        print_bytes("invalid online response", online_response, DPI_REPORT_SIZE);
        return -1;
    }
    if (require_online && online_response[6] == 0) {
        print_bytes("mouse is not reporting online", online_response, DPI_REPORT_SIZE);
        return -1;
    }
    return 0;
}

static int set_sensitivity(libusb_device_handle *handle, int sens, unsigned char override) {
    if (check_online(handle, false) != 0) {
        return 1;
    }
    struct mouse_state initial;
    if (read_mouse_state(handle, &initial) != 0) {
        return 1;
    }
    unsigned char address = override ? override : initial.address;
    unsigned int target_stage = (address - DPI_ADDRESS_BASE) / DPI_STAGE_SIZE;
    if (target_stage >= initial.config[2]) {
        fprintf(stderr, "debug target is outside the device's configured DPI stages\n");
        return 1;
    }
    printf("profile=%u index=%u\n", initial.profile + 1, initial.profile);
    printf("stage=%u/%u index=%u\n", initial.config[4] + 1, initial.config[2], initial.config[4]);
    printf("target=0x%02x%s\n", address, override ? " debug=yes" : "");

    unsigned char stored[DPI_STAGE_SIZE];
    if (read_register(handle, address, DPI_STAGE_SIZE, stored) != 0) {
        return 1;
    }
    unsigned int sum = stored[0] + stored[1] + stored[2] + stored[3];
    unsigned int x = (stored[0] + ((stored[2] >> 2) & 3) * 256 + 1) * 50;
    unsigned int y = (stored[1] + ((stored[2] >> 6) & 3) * 256 + 1) * 50;
    if ((sum & 0xff) != 0x55 || (stored[2] & 0x33) != 0 || x > 26000 || y > 26000) {
        print_bytes("unsupported DPI register data", stored, DPI_STAGE_SIZE);
        return 1;
    }
    if (x == y) {
        printf("new=%u\n", x);
    } else {
        printf("new.X=%u new.Y=%u\n", x, y);
    }

    struct mouse_state before;
    if (read_mouse_state(handle, &before) != 0 || !same_state(&initial, &before)) {
        fprintf(stderr, "profile/stage could not be confirmed before write; no setting written\n");
        return 1;
    }
    unsigned char payload[DPI_REPORT_SIZE];
    unsigned char response[DPI_REPORT_SIZE];
    if (get_payload(sens, address, payload) != 0 || exchange(handle, payload, response) != 0) {
        return 1;
    }
    printf("ACK=yes\n");

    if (read_register(handle, address, DPI_STAGE_SIZE, stored) != 0) {
        return 1;
    }
    if (memcmp(stored, payload + 6, DPI_STAGE_SIZE) != 0) {
        print_bytes("DPI register readback mismatch", stored, DPI_STAGE_SIZE);
        return 1;
    }
    struct mouse_state after;
    if (read_mouse_state(handle, &after) != 0 || !same_state(&initial, &after)) {
        fprintf(stderr, "profile/stage changed or could not be confirmed after write\n");
        return 1;
    }
    printf("readback=%d\n", sens);
    if (address != initial.address) {
        printf("debug write targeted an inactive stage\n");
    }
    return 0;
}

static libusb_device_handle *open_mouse(bool wired_only) {
    libusb_device_handle *handle = NULL;
    int r = libusb_init(NULL);
    if (r < 0) {
        fprintf(stderr, "libusb init failed: %s\n", libusb_error_name(r));
        return NULL;
    }

    handle = libusb_open_device_with_vid_pid(NULL, TARGET_VID, TARGET_PID_WIRED);
    if (!handle && !wired_only) {
        handle = libusb_open_device_with_vid_pid(NULL, TARGET_VID, TARGET_PID_WIRELESS);
    }
    if (!handle) {
        fprintf(stderr, "%s\n",
                wired_only ? "driver: u have to be plugged to use this feature"
                           : "mouse not found!!");
        libusb_exit(NULL);
        return NULL;
    }
    printf("mouse found!!\n");
    r = libusb_set_auto_detach_kernel_driver(handle, 1);
    if (r < 0) {
        fprintf(stderr, "couldnt enable kernel driver reattachment: %s\n", libusb_error_name(r));
        libusb_close(handle);
        libusb_exit(NULL);
        return NULL;
    }
    int claimed = libusb_claim_interface(handle, 1);
    if (claimed < 0) {
        fprintf(stderr, "couldnt claim interface 1: %s\n", libusb_error_name(claimed));
        libusb_close(handle);
        libusb_exit(NULL);
        return NULL;
    }

    return handle;
}

static int close_mouse(libusb_device_handle *handle, int status) {
    int r = libusb_release_interface(handle, 1);
    if (r < 0) {
        fprintf(stderr, "couldnt release interface 1: %s\n", libusb_error_name(r));
        status = 1;
    }
    libusb_close(handle);
    libusb_exit(NULL);
    return status;
}

int run_driver_at_address(int sens, unsigned char dpi_address) {
    if (sens < 50 || sens > 26000 || sens % 50 != 0 ||
        (dpi_address && (dpi_address < DPI_ADDRESS_BASE ||
                         dpi_address >= DPI_ADDRESS_BASE + DPI_MAX_STAGES * DPI_STAGE_SIZE ||
                         (dpi_address - DPI_ADDRESS_BASE) % DPI_STAGE_SIZE != 0))) {
        fprintf(stderr, "invalid sensitivity or DPI address\n");
        return 1;
    }
    printf("requested DPI: %d\n", sens);
    libusb_device_handle *handle = open_mouse(false);
    if (!handle) {
        return 1;
    }
    return close_mouse(handle, set_sensitivity(handle, sens, dpi_address));
}

int run_battery(void) {
    libusb_device_handle *handle = open_mouse(true);
    if (!handle) {
        return 1;
    }
    unsigned char request[DPI_REPORT_SIZE] = {[0] = 0x08, [1] = 0x04, [16] = 0x49};
    unsigned char response[DPI_REPORT_SIZE];
    int status = 1;
    if (check_online(handle, true) == 0 && exchange(handle, request, response) == 0) {
        struct battery_info battery;
        if (decode_battery_report(response, &battery) == 0) {
            unsigned int full = battery.percentage / 10;
            unsigned int remainder = battery.percentage % 10;
            const char *partial = remainder >= 8 ? "⣷" : remainder >= 5 ? "⣤" : "⡀";
            fputs("battery: ", stdout);
            for (unsigned int cell = 0; cell < 10; cell++) {
                fputs(cell < full                              ? "⣿"
                      : cell == full && battery.percentage > 0 ? partial
                                                               : "⠀",
                      stdout);
            }
            printf(" %3u%% (%s; wired%s)\n", battery.percentage,
                   battery.charging ? "charging" : "not charging",
                   battery.charging ? "; device estimate" : "");
            status = 0;
        } else {
            print_bytes("invalid battery response", response, DPI_REPORT_SIZE);
        }
    }
    return close_mouse(handle, status);
}

int run_driver(int sens) { return run_driver_at_address(sens, 0); }
