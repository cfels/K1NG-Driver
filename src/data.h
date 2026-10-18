#ifndef DATA_H
#define DATA_H

#include <stdbool.h>

#define DPI_REPORT_SIZE 17
#define DPI_ADDRESS_BASE 0x0c
#define DPI_STAGE_SIZE 4
#define DPI_MAX_STAGES 8
#define DPI_CONFIG_SIZE 6

struct battery_info {
    unsigned char percentage;
    bool charging;
};

int get_payload(int level, unsigned char address, unsigned char *buf);
int get_read_payload(unsigned char address, unsigned char length, unsigned char *buf);
void get_profile_payload(unsigned char *buf);
int resolve_dpi_target(const unsigned char *config, unsigned char *address);
int decode_battery_report(const unsigned char *report, struct battery_info *battery);

#endif
