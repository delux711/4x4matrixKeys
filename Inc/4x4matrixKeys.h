#ifndef _4X4MATRIXKEYS_H
#define _4X4MATRIXKEYS_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32l4xx.h"

/*
Pins:
     C0 C1 C2 C3
R0    1  2  3  A
R1    4  5  6  B
R2    7  8  9  C
R3    *  0  #  D

R0, R1, R2, R3, C0, C1, C2, C3
*/

/*
// Stm32L432KC default pins
#define R0_PIN  4
#define R0_PORT B

#define R1_PIN  5
#define R1_PORT B

#define R2_PIN  11
#define R2_PORT A

#define R3_PIN  8
#define R3_PORT A

#define C0_PIN  15
#define C0_PORT C

#define C1_PIN  14
#define C1_PORT C

#define C2_PIN  1
#define C2_PORT B

#define C3_PIN  6
#define C3_PORT B
*/

#define R0_PIN  4
#define R0_PORT B

#define R1_PIN  5
#define R1_PORT B

#define R2_PIN  11
#define R2_PORT A

#define R3_PIN  8
#define R3_PORT A

#define C0_PIN  15
#define C0_PORT C

#define C1_PIN  14
#define C1_PORT C

#define C2_PIN  1
#define C2_PORT B

#define C3_PIN  6
#define C3_PORT B

#define MKEY_TIME_NO_PRESS  (255u)
#define MKEY_TIME_PRESS     (255u)

/*****************************************************************************************************************/

typedef enum _MKEY_state_e {
    MKEY_STATE_NOT_INIT,
    MKEY_STATE_IDLE,
    MKEY_STATE_ERROR,
    MKEY_STATE_PRESS,
    MKEY_STATE_NO_PRESS,
    MKEY_STATE_SCAN
} MKEY_state_e;

extern MKEY_state_e MKEYS_vHandleTask(void);
extern uint8_t MKEYS_ucGetKey(void);
extern uint8_t MKEYS_ucGetLastKey(void);
extern bool MKEYS_bIsActualPress(void);
extern bool MKEYS_bIsPress(void);

#endif
