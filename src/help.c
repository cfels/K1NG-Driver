#include "../includes/include.h"
#include "help.h"

void intro(void) {
    printf("K1NG PRO (4k) driver | Cli\n");
    printf("set any sens from 50-26000\n\n");
    printf("rememeber to set an value like in original software so for example `50 + 50 = 100` or `100 + 50 = 150` u get the point it goes up by 50\n\n");
}

void help(void) {
    intro();
    printf("usage:\n");
    printf(" sudo ./k1ng_driver               interactive prompt\n");
    printf(" sudo ./k1ng_driver <sens>        set sens directly\n");
    printf(" sudo ./k1ng_driver -p <preset>   load sens from a saved preset\n");
    printf(" sudo ./k1ng_driver -h            show's help duh!\n");
    printf(" sudo ./k1ng_driver -s                 to save a new preset\n");
}
