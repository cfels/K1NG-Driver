#include "includes/include.h"
#include "src/driver.h"

int main(int argc, char *argv[]) {
    int sens;

    printf("K1NG PRO (4k) driver | Cli\n");
    printf("set any sens from 50-26000\n\n");
    printf("rememeber to set an value like in original software so for example `50 + 50 = 100` or `100 + 50 = 150` u get the point it goes up by 50\n\n");

    if (argc < 2) {
        printf("set sensitivity to: ");
        if (scanf("%d", &sens) != 1) {
            printf("u forgot to input a number?\n");
            return 1;
        }
    } else {
        sens = atoi(argv[1]);
        printf("set sensitivity to: %d\n", sens);
    }

    run_driver(sens);
    return 0;
}
