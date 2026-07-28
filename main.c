#include "includes/include.h"
#include "src/driver.h"

int main(int argc, char *argv[]) {
    int level;

    printf("K1NG PRO (4k) driver | Cli\n");
    printf("set any sens from 50-26000\n\n");

    if (argc < 2) {
        printf("set sensitivity to: ");
        if (scanf("%d", &level) != 1) {
            printf("u forgot to input a number?\n");
            return 1;
        }
    } else {
        level = atoi(argv[1]);
        printf("set sensitivity to: %d\n", level);
    }

    run_driver(level);
    return 0;
}
