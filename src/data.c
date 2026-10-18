#include "data.h"
#include <string.h>

static void finish_report(unsigned char *buf) {
    unsigned int sum = 0;
    for (int i = 0; i < DPI_REPORT_SIZE - 1; i++) {
        sum += buf[i];
    }
    buf[DPI_REPORT_SIZE - 1] = (unsigned char)(0x55 - sum);
}

int get_payload(int level, unsigned char address, unsigned char *buf) {
    if (!buf || level < 50 || level > 26000 || level % 50 != 0 || address < DPI_ADDRESS_BASE ||
        address >= DPI_ADDRESS_BASE + DPI_MAX_STAGES * DPI_STAGE_SIZE ||
        (address - DPI_ADDRESS_BASE) % DPI_STAGE_SIZE != 0) {
        return -1;
    }

    unsigned int value = (unsigned int)(level / 50 - 1);
    memset(buf, 0, DPI_REPORT_SIZE);
    buf[0] = 0x08;
    buf[1] = 0x07;
    buf[4] = address;
    buf[5] = 0x04;
    buf[6] = value & 0xff;
    buf[7] = value & 0xff;
    buf[8] = (value >> 8) * 0x44;
    buf[9] = (unsigned char)(0x55 - buf[6] - buf[7] - buf[8]);

    finish_report(buf);
    return 0;
}

int get_read_payload(unsigned char address, unsigned char length, unsigned char *buf) {
    if (!buf || length == 0 || length > 10 || address + length > 256) {
        return -1;
    }
    memset(buf, 0, DPI_REPORT_SIZE);
    buf[0] = 0x08;
    buf[1] = 0x08;
    buf[4] = address;
    buf[5] = length;
    finish_report(buf);
    return 0;
}

void get_profile_payload(unsigned char *buf) {
    memset(buf, 0, DPI_REPORT_SIZE);
    buf[0] = 0x08;
    buf[1] = 0x0e;
    finish_report(buf);
}

int resolve_dpi_target(const unsigned char *config, unsigned char *address) {
    if (!config || !address) {
        return -1;
    }
    for (int i = 0; i < DPI_CONFIG_SIZE; i += 2) {
        if ((unsigned char)(config[i] + config[i + 1]) != 0x55) {
            return -1;
        }
    }
    unsigned char polling = config[0];
    unsigned char count = config[2];
    unsigned char stage = config[4];
    if (polling == 0 || polling > 0x40 || (polling & (polling - 1)) != 0 || count == 0 ||
        count > DPI_MAX_STAGES || stage >= count) {
        return -1;
    }
    *address = DPI_ADDRESS_BASE + stage * DPI_STAGE_SIZE;
    return 0;
}

int decode_battery_report(const unsigned char *report, struct battery_info *battery) {
    if (!report || !battery || report[0] != 0x08 || report[1] != 0x04 || report[2] != 0 ||
        report[3] != 0 || report[4] != 0 || report[5] < 2 || report[5] > 10 || report[6] > 100 ||
        report[7] > 1) {
        return -1;
    }
    unsigned int sum = 0;
    for (int i = 0; i < DPI_REPORT_SIZE; i++) {
        sum += report[i];
    }
    if ((sum & 0xff) != 0x55) {
        return -1;
    }
    battery->percentage = report[6];
    battery->charging = report[7] != 0;
    return 0;
}
