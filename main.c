#include "includes/include.h"
#include "src/driver.h"
#include "src/config.h"
#include "src/help.h"

int main(int argc, char *argv[]) {
    int sens;
    char input[100];

    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        help();
        return 0;
    }

    intro();

    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/mouse_presets.toml", getenv("HOME"));

    if (argc >= 2 && strcmp(argv[1], "-s") == 0) {
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
        int dpi = atoi(dpi_str);

        if (dpi < 50 || dpi > 26000 || dpi % 50 != 0) {
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

        config_save_preset(config_path, name, dpi);
        printf("saved preset '%s' with dpi %d\n", name, dpi);
        return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "-p") == 0) {
        if (argc < 3) {
            printf("usage: ./k1ng_driver -p <preset>\n");
            return 1;
        }

        char *preset_name = argv[2];

        if (config_load_preset(config_path, preset_name, &sens) != 0) {
            printf("couldnt load preset '%s'\n", preset_name);
            return 1;
        }
        printf("loaded preset '%s' sens: %d\n", preset_name, sens);
    } else if (argc < 2) {
        printf("set sensitivity to: ");
        if (fgets(input, sizeof(input), stdin) == NULL || input[0] == '\n') {
            printf("incorrect input press enter to try again\n");
            return 1;
        }
        char *endptr;
        sens = (int)strtol(input, &endptr, 10);
        if (*endptr != '\n' && *endptr != '\0') {
            printf("incorrect input press enter to try again\n");
            return 1;
        }
    } else {
        sens = atoi(argv[1]);
        printf("set sensitivity to: %d\n", sens);
    }

    if (sens < 50 || sens > 26000 || sens % 50 != 0) {
        printf("incorrect input press enter to try again\n");
        return 1;
    }

    run_driver(sens);
    return 0;
}
