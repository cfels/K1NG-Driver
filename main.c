#include "includes/include.h"
#include "src/driver.h"
#include "src/config.h"
#include "src/help.h"
#include "src/data.h"
#include <errno.h>

static int parse_sensitivity(const char *text, int *dpi) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno != 0 || end == text || (*end != '\0' && !(end[0] == '\n' && end[1] == '\0')) ||
        value < 50 || value > 26000 || value % 50 != 0) {
        return -1;
    }
    *dpi = (int)value;
    return 0;
}

int main(int argc, char *argv[]) {
    int sens;
    char input[100];
    int arg = 1;
    unsigned char dpi_address = 0;

    if (argc > arg && strcmp(argv[arg], "--dpi-address") == 0) {
        if (argc <= arg + 1) {
            fprintf(stderr, "--dpi-address requires an aligned stage address from 0x0c to 0x28\n");
            return 1;
        }
        char *end;
        errno = 0;
        long address = strtol(argv[arg + 1], &end, 0);
        if (errno != 0 || end == argv[arg + 1] || *end != '\0' || address < DPI_ADDRESS_BASE ||
            address >= DPI_ADDRESS_BASE + DPI_MAX_STAGES * DPI_STAGE_SIZE ||
            (address - DPI_ADDRESS_BASE) % DPI_STAGE_SIZE != 0) {
            fprintf(stderr, "--dpi-address requires an aligned stage address from 0x0c to 0x28\n");
            return 1;
        }
        dpi_address = (unsigned char)address;
        arg += 2;
    }

    if (argc > arg && (strcmp(argv[arg], "-h") == 0 || strcmp(argv[arg], "--help") == 0)) {
        help();
        return 0;
    }

    if (argc > arg && (strcmp(argv[arg], "--battery") == 0 || strcmp(argv[arg], "-b") == 0 ||
                       strcmp(argv[arg], "-bat") == 0)) {
        if (argc != arg + 1 || dpi_address != 0) {
            fprintf(stderr, "%s takes no additional arguments\n", argv[arg]);
            return 1;
        }
        return run_battery();
    }

    intro();

    if (argc > arg + 1 && !(strcmp(argv[arg], "-p") == 0 && argc == arg + 2)) {
        fprintf(stderr, "unexpected arguments; see --help\n");
        return 1;
    }

    char config_path[512];
    if (argc > arg && (strcmp(argv[arg], "-s") == 0 || strcmp(argv[arg], "-p") == 0)) {
        const char *home = getenv("HOME");
        if (!home || snprintf(config_path, sizeof(config_path), "%s/mouse_presets.toml", home) >=
                         (int)sizeof(config_path)) {
            fprintf(stderr, "cannot determine preset path\n");
            return 1;
        }
    }

    if (argc > arg && strcmp(argv[arg], "-s") == 0) {
        char name[128];
        char dpi_str[32];
        char confirm[8];

        printf("preset name: ");
        if (fgets(name, sizeof(name), stdin) == NULL) {
            printf("incorrect input press enter to try again\n");
            return 1;
        }
        name[strcspn(name, "\n")] = 0;

        printf("dpi: ");
        if (fgets(dpi_str, sizeof(dpi_str), stdin) == NULL) {
            printf("incorrect input press enter to try again\n");
            return 1;
        }
        int dpi;
        if (parse_sensitivity(dpi_str, &dpi) != 0) {
            printf("incorrect input press enter to try again\n");
            return 1;
        }

        printf("do you really want to save sens: %d, name: '%s'? (y/n): ", dpi, name);
        if (fgets(confirm, sizeof(confirm), stdin) == NULL) {
            printf("incorrect input press enter to try again\n");
            return 1;
        }

        if (confirm[0] != 'y' && confirm[0] != 'Y') {
            printf("cancelled by user!\n");
            return 0;
        }

        if (config_save_preset(config_path, name, dpi) != 0) {
            fprintf(stderr, "couldnt save preset '%s'\n", name);
            return 1;
        }
        printf("saved preset '%s' with dpi %d\n", name, dpi);
        return 0;
    }

    if (argc > arg && strcmp(argv[arg], "-p") == 0) {
        if (argc <= arg + 1) {
            printf("usage: ./k1ng_driver -p <preset>\n");
            return 1;
        }

        char *preset_name = argv[arg + 1];

        if (config_load_preset(config_path, preset_name, &sens) != 0) {
            printf("couldnt load preset '%s'\n", preset_name);
            return 1;
        }
        printf("loaded preset '%s' sens: %d\n", preset_name, sens);
    } else if (argc <= arg) {
        printf("set sensitivity to: ");
        if (fgets(input, sizeof(input), stdin) == NULL || input[0] == '\n') {
            printf("incorrect input press enter to try again\n");
            return 1;
        }
        if (parse_sensitivity(input, &sens) != 0) {
            printf("incorrect input press enter to try again\n");
            return 1;
        }
    } else {
        if (parse_sensitivity(argv[arg], &sens) != 0) {
            fprintf(stderr, "sensitivity must be 50-26000 in steps of 50\n");
            return 1;
        }
        printf("set sensitivity to: %d\n", sens);
    }

    if (sens < 50 || sens > 26000 || sens % 50 != 0) {
        printf("incorrect input press enter to try again\n");
        return 1;
    }

    return dpi_address ? run_driver_at_address(sens, dpi_address) : run_driver(sens);
}
