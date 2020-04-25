#include <stdint.h>
#include <stdbool.h>
#include "4x4matrixKeys.h"

static uint8_t button;

int main() {
    button = 0;
    for(;;) {
        MKEYS_vHandleTask();
        if(MKEYS_bIsPress()) {
            button = MKEYS_ucGetKey();
        }
        (void)button;
    }
    return 0u;
}
