#include "includes/include.h"
#include "src/driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int sens;
    char input[100];

    printf("K1NG PRO (4k) driver | Cli\n");
    printf("set any sens from 50-26000\n\n");
    printf("rememeber to set an value like in original software so for example `50 + 50 = 100` or `100 + 50 = 150` u get the point it goes up by 50\n\n");

    if (argc < 2) {
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
