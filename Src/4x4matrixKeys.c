#include "4x4matrixKeys.h"

/*
Pins:
     C0 C1 C2 C3
R0    1  2  3  A
R1    4  5  6  B
R2    7  8  9  C
R3    *  0  #  D

R0, R1, R2, R3, C0, C1, C2, C3
*/
typedef enum _MKEY_pins_row_state_e {
    MKEY_PINS_ROW_STATE_0,
    MKEY_PINS_ROW_STATE_1,
    MKEY_PINS_ROW_STATE_2,
    MKEY_PINS_ROW_STATE_3,
    MKEY_PINS_ROW_STATE_4
} MKEY_pins_row_state_e;

#define MKEY_ERROR  (0xFFu)


static bool mkey_bIsPress = false;
static uint8_t mkey_ucPress = 0u;
static uint8_t mkey_ucPressLast = 0u;
static uint8_t mkey_ucTimer = 0u;
static uint8_t mkey_ucTimerNoPress = 0u;
static volatile MKEY_state_e mkey_state = MKEY_STATE_NOT_INIT;
static uint8_t mkey_pData[4][4] = { { '1', '2', '3', 'A'},
                                    { '4', '5', '6', 'B'},
                                    { '7', '8', '9', 'C'},
                                    { '*', '0', '#', 'D'}};

void MKEY_vInitPort(void);
void MKEY_vSetRowsState(MKEY_pins_row_state_e state);
void MKEY_vInitExtInterrupts(void);
void MKEY_vEnableInterrupts(void);
void MKEY_vDisableInterrupts(void);
void MKEY_vIRQHandler(void);
uint8_t MKEY_ucGetColumn(void);

// Pins macros
#define MKEY_OUTPUT                         (1u)
#define MKEY_INPUT                          (0u)
#define _MKEY_vEnablePort(port)             RCC->AHB2ENR |= RCC_AHB2ENR_GPIO##port##EN;
#define _MKEY_vSetDir(inOut,port,pin)       do {                                                                      \
                                                 uint32_t tempCR = GPIO##port->MODER & (~(GPIO_MODER_MODE##pin));     \
                                                 tempCR |= (inOut << GPIO_MODER_MODE##pin##_Pos);                     \
                                                 GPIO##port->MODER = tempCR;                                          \
                                             } while(0)
#define _MKEY_vInitOpenDrain(port,pin)      do { GPIO##port->OTYPER |= GPIO_OTYPER_OT##pin##_Msk; } while(0)
#define _MKEY_bGetPin(port,pin)             ((GPIO##port->IDR & GPIO_IDR_ID##pin) != 0u)
#define _MKEY_vInitPullUp(port,pin)         do { GPIO##port->PUPDR |= (1u << GPIO_PUPDR_PUPD##pin##_Pos); } while(0)
#define _MKEY_vSetPin(log,port,pin)         do { if(log) { GPIO##port->BSRR |= GPIO_BSRR_BS##pin; } else { GPIO##port->BSRR |= GPIO_BSRR_BR##pin; } }while(0)

#define MKEY_vSetDir(inOut,port,pin)        _MKEY_vSetDir(inOut,port,pin)
#define MKEY_vSetDirR0(inOut)               MKEY_vSetDir(inOut, R0_PORT, R0_PIN)
#define MKEY_vSetDirR1(inOut)               MKEY_vSetDir(inOut, R1_PORT, R1_PIN)
#define MKEY_vSetDirR2(inOut)               MKEY_vSetDir(inOut, R2_PORT, R2_PIN)
#define MKEY_vSetDirR3(inOut)               MKEY_vSetDir(inOut, R3_PORT, R3_PIN)
#define MKEY_vSetDirC0(inOut)               MKEY_vSetDir(inOut, C0_PORT, C0_PIN)
#define MKEY_vSetDirC1(inOut)               MKEY_vSetDir(inOut, C1_PORT, C1_PIN)
#define MKEY_vSetDirC2(inOut)               MKEY_vSetDir(inOut, C2_PORT, C2_PIN)
#define MKEY_vSetDirC3(inOut)               MKEY_vSetDir(inOut, C3_PORT, C3_PIN)

#define MKEY_vInitOpenDrain(port,pin)       _MKEY_vInitOpenDrain(port,pin)
#define MKEY_vInitPullUp(port,pin)          _MKEY_vInitPullUp(port,pin)

#define MKEY_vEnablePort(port)              _MKEY_vEnablePort(port)
#define MKEY_bGetPin(port,pin)              _MKEY_bGetPin(port,pin)
#define MKEY_bGetPinC0()                    MKEY_bGetPin(C0_PORT, C0_PIN)
#define MKEY_bGetPinC1()                    MKEY_bGetPin(C1_PORT, C1_PIN)
#define MKEY_bGetPinC2()                    MKEY_bGetPin(C2_PORT, C2_PIN)
#define MKEY_bGetPinC3()                    MKEY_bGetPin(C3_PORT, C3_PIN)
#define MKEY_vSetPin(log,port,pin)          _MKEY_vSetPin(log,port,pin)

// Interrupts macros
#define __MKEY_EXTI_PINS_Msk(pinA, pinB, pinC, pinD)  ((uint32_t)(EXTI_IMR1_IM##pinA##_Msk | EXTI_IMR1_IM##pinB##_Msk | EXTI_IMR1_IM##pinC##_Msk | EXTI_IMR1_IM##pinD##_Msk))
#define _MKEY_EXTI_PINS_Msk(pinA, pinB, pinC, pinD)   __MKEY_EXTI_PINS_Msk(pinA, pinB, pinC, pinD)
#define MKEY_EXTI_PINS_Msk                            _MKEY_EXTI_PINS_Msk(C0_PIN, C1_PIN, C2_PIN, C3_PIN)

#if(C0_PIN < 4u)
#define MKEY_EXTI_PIN_0_EXTICR 1
#elif(C0_PIN < 8u)
#define MKEY_EXTI_PIN_0_EXTICR 2
#elif(C0_PIN < 12u)
#define MKEY_EXTI_PIN_0_EXTICR 3
#elif(C0_PIN < 16u)
#define MKEY_EXTI_PIN_0_EXTICR 4
#else
    #error "Pin C0_PIN not supported"
#endif
#if(C1_PIN < 4u)
#define MKEY_EXTI_PIN_1_EXTICR 1
#elif(C1_PIN < 8u)
#define MKEY_EXTI_PIN_1_EXTICR 2
#elif(C1_PIN < 12u)
#define MKEY_EXTI_PIN_1_EXTICR 3
#elif(C1_PIN < 16u)
#define MKEY_EXTI_PIN_1_EXTICR 4
#else
    #error "Pin C1_PIN not supported"
#endif
#if(C2_PIN < 4u)
#define MKEY_EXTI_PIN_2_EXTICR 1
#elif(C2_PIN < 8u)
#define MKEY_EXTI_PIN_2_EXTICR 2
#elif(C2_PIN < 12u)
#define MKEY_EXTI_PIN_2_EXTICR 3
#elif(C2_PIN < 16u)
#define MKEY_EXTI_PIN_2_EXTICR 4
#else
    #error "Pin C2_PIN not supported"
#endif
#if(C3_PIN < 4u)
#define MKEY_EXTI_PIN_3_EXTICR 1
#elif(C3_PIN < 8u)
#define MKEY_EXTI_PIN_3_EXTICR 2
#elif(C3_PIN < 12u)
#define MKEY_EXTI_PIN_3_EXTICR 3
#elif(C3_PIN < 16u)
#define MKEY_EXTI_PIN_3_EXTICR 4
#else
    #error "Pin C3_PIN not supported"
#endif

#define __MKEY_EXTI_PINS_EXTICR_PORT(exticr, port, pin)     SYSCFG_EXTICR##exticr##_EXTI##pin##_P##port
#define _MKEY_EXTI_PINS_EXTICR_PORT(exticr, port, pin)     __MKEY_EXTI_PINS_EXTICR_PORT(exticr, port, pin)
#define MKEY_EXTI_PIN_0_EXTICR_PORT                 _MKEY_EXTI_PINS_EXTICR_PORT(MKEY_EXTI_PIN_0_EXTICR, C0_PORT, C0_PIN)
#define MKEY_EXTI_PIN_1_EXTICR_PORT                 _MKEY_EXTI_PINS_EXTICR_PORT(MKEY_EXTI_PIN_1_EXTICR, C1_PORT, C1_PIN)
#define MKEY_EXTI_PIN_2_EXTICR_PORT                 _MKEY_EXTI_PINS_EXTICR_PORT(MKEY_EXTI_PIN_2_EXTICR, C2_PORT, C2_PIN)
#define MKEY_EXTI_PIN_3_EXTICR_PORT                 _MKEY_EXTI_PINS_EXTICR_PORT(MKEY_EXTI_PIN_3_EXTICR, C3_PORT, C3_PIN)

#define __MKEY_EXTI_INT(num)  EXTI##num##_IRQn
#define _MKEY_EXTI_INT(num)  __MKEY_EXTI_INT(num)
#if(C0_PIN < 5u)
// EXTI0_IRQn .. EXTI4_IRQn
#define MKEY_EXTI_INT0  _MKEY_EXTI_INT(C0_PIN)
#elif(C0_PIN <= 9)
// EXTI9_5_IRQn
#define MKEY_EXTI_INT0  EXTI9_5_IRQn
#elif(C0_PIN <= 15)
// EXTI15_10_IRQn
#define MKEY_EXTI_INT0  EXTI15_10_IRQn
#else
    #error "Pin C0_PIN not supported in interrupt"
#endif
#if(C1_PIN < 5u)
#define MKEY_EXTI_INT1  _MKEY_EXTI_INT(C1_PIN)
#elif(C1_PIN <= 9)
#define MKEY_EXTI_INT1  EXTI9_5_IRQn
#elif(C1_PIN <= 15)
#define MKEY_EXTI_INT1  EXTI15_10_IRQn
#else
    #error "Pin C1_PIN not supported in interrupt"
#endif
#if(C2_PIN < 5u)
#define MKEY_EXTI_INT2  _MKEY_EXTI_INT(C2_PIN)
#elif(C2_PIN <= 9)
#define MKEY_EXTI_INT2  EXTI9_5_IRQn
#elif(C2_PIN <= 15)
#define MKEY_EXTI_INT2  EXTI15_10_IRQn
#else
    #error "Pin C2_PIN not supported in interrupt"
#endif
#if(C3_PIN < 5u)
#define MKEY_EXTI_INT3  _MKEY_EXTI_INT(C3_PIN)
#elif(C3_PIN <= 9)
#define MKEY_EXTI_INT3  EXTI9_5_IRQn
#elif(C3_PIN <= 15)
#define MKEY_EXTI_INT3  EXTI15_10_IRQn
#else
    #error "Pin C3_PIN not supported in interrupt"
#endif


uint8_t MKEYS_ucGetKey(void) {
    mkey_bIsPress = false;
    return mkey_ucPress;
}

bool MKEYS_bIsPress(void) {
    return mkey_bIsPress;
}

void MKEY_vInitPort(void) {
    // ENABLE PORT
    MKEY_vEnablePort(R0_PORT);
    MKEY_vEnablePort(R1_PORT);
    MKEY_vEnablePort(R2_PORT);
    MKEY_vEnablePort(R3_PORT);
    MKEY_vEnablePort(C0_PORT);
    MKEY_vEnablePort(C1_PORT);
    MKEY_vEnablePort(C2_PORT);
    MKEY_vEnablePort(C3_PORT);
    // 0 for all rows
    MKEY_vSetRowsState(MKEY_PINS_ROW_STATE_0);
    // OPEN DRAIN for rows
    MKEY_vInitOpenDrain(R0_PORT, R0_PIN);
    MKEY_vInitOpenDrain(R1_PORT, R1_PIN);
    MKEY_vInitOpenDrain(R2_PORT, R2_PIN);
    MKEY_vInitOpenDrain(R3_PORT, R3_PIN);
    // PULL UP for column
    MKEY_vInitPullUp(C0_PORT, C0_PIN);
    MKEY_vInitPullUp(C1_PORT, C1_PIN);
    MKEY_vInitPullUp(C2_PORT, C2_PIN);
    MKEY_vInitPullUp(C3_PORT, C3_PIN);
    // DIRECTION
    MKEY_vSetDirR0(MKEY_OUTPUT);
    MKEY_vSetDirR1(MKEY_OUTPUT);
    MKEY_vSetDirR2(MKEY_OUTPUT);
    MKEY_vSetDirR3(MKEY_OUTPUT);
    MKEY_vSetDirC0(MKEY_INPUT);
    MKEY_vSetDirC1(MKEY_INPUT);
    MKEY_vSetDirC2(MKEY_INPUT);
    MKEY_vSetDirC3(MKEY_INPUT);   
}

/*
State of row outputs:
    1. 2. 3. 4. 5. 6. 7.
    0  0  Z  Z  Z  Z  0
    0  0  Z  Z  Z  0  Z
    0  Z  0  Z  0  Z  Z
    0  Z  0  0  Z  Z  Z

0-output with log.0
Z-open drain output (high impedance)
*/
void MKEY_vSetRowsState(MKEY_pins_row_state_e state) {
    switch(state) {
        case 0:
            MKEY_vSetPin(0, R0_PORT, R0_PIN);
            MKEY_vSetPin(0, R1_PORT, R1_PIN);
            MKEY_vSetPin(0, R2_PORT, R2_PIN);
            MKEY_vSetPin(0, R3_PORT, R3_PIN);
            break;
        case 1:
            MKEY_vSetPin(0, R0_PORT, R0_PIN);
            MKEY_vSetPin(1, R1_PORT, R1_PIN);
            MKEY_vSetPin(1, R2_PORT, R2_PIN);
            MKEY_vSetPin(1, R3_PORT, R3_PIN);
            break;
        case 2:
            MKEY_vSetPin(1, R0_PORT, R0_PIN);
            MKEY_vSetPin(0, R1_PORT, R1_PIN);
            MKEY_vSetPin(1, R2_PORT, R2_PIN);
            MKEY_vSetPin(1, R3_PORT, R3_PIN);
            break;
        case 3:
            MKEY_vSetPin(1, R0_PORT, R0_PIN);
            MKEY_vSetPin(1, R1_PORT, R1_PIN);
            MKEY_vSetPin(0, R2_PORT, R2_PIN);
            MKEY_vSetPin(1, R3_PORT, R3_PIN);
            break;
        case 4:
            MKEY_vSetPin(1, R0_PORT, R0_PIN);
            MKEY_vSetPin(1, R1_PORT, R1_PIN);
            MKEY_vSetPin(1, R2_PORT, R2_PIN);
            MKEY_vSetPin(0, R3_PORT, R3_PIN);
            break;
    }
}

void MKEY_vInitExtInterrupts(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN_Msk;
    // 0-0..3  1-4..7  2-8..11  3-12..15
    SYSCFG->EXTICR[MKEY_EXTI_PIN_0_EXTICR-1u] |= MKEY_EXTI_PIN_0_EXTICR_PORT;    
    SYSCFG->EXTICR[MKEY_EXTI_PIN_1_EXTICR-1u] |= MKEY_EXTI_PIN_1_EXTICR_PORT;
    SYSCFG->EXTICR[MKEY_EXTI_PIN_2_EXTICR-1u] |= MKEY_EXTI_PIN_2_EXTICR_PORT;
    SYSCFG->EXTICR[MKEY_EXTI_PIN_3_EXTICR-1u] |= MKEY_EXTI_PIN_3_EXTICR_PORT;
    
    /*
    Pins C14 C15, B1 B6
    // 0-0..3  1-4..7  2-8..11  3-12..15
    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI14_PC; // C14
    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI15_PC; // C15
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PB;  // B1
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI6_PB;  // B6
    */
    
    EXTI->FTSR1 |= MKEY_EXTI_PINS_Msk; // Ext int set to falling edge
    MKEY_vEnableInterrupts();   // Enable ext event

    NVIC_EnableIRQ(MKEY_EXTI_INT0);
    NVIC_EnableIRQ(MKEY_EXTI_INT1);
    NVIC_EnableIRQ(MKEY_EXTI_INT2);
    NVIC_EnableIRQ(MKEY_EXTI_INT3);
    /*
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    */
}

void MKEY_vEnableInterrupts(void) {
    EXTI->IMR1 |= MKEY_EXTI_PINS_Msk;   // Enable ext event
}
void MKEY_vDisableInterrupts(void) {
    EXTI->IMR1 &= ~MKEY_EXTI_PINS_Msk;  // Disable ext event
}

bool MKEY_bGetScan(uint8_t ucSetRow, MKEY_pins_row_state_e eTestState, uint8_t* pucPress) {
    uint8_t ucPress;
    MKEY_vSetRowsState(eTestState);
    ucPress = MKEY_ucGetColumn();
    if(ucPress == MKEY_ERROR) {
        return false;
    } else {
        *pucPress = mkey_pData[ucSetRow][ucPress];
    }
    return true;
}

uint8_t MKEY_ucGetColumn(void) {
    uint8_t ret = MKEY_ERROR;
    if(MKEY_bGetPinC0() && MKEY_bGetPinC1()) {
        if(MKEY_bGetPinC2() == false) {
            ret = 2u;
        } else {
            if(MKEY_bGetPinC3() == false) {
                ret = 3u;
            }
        }
    } else {
        if(MKEY_bGetPinC0() == false) {
            ret = 0u;
        } else if(MKEY_bGetPinC1() == false) {
            ret = 1u;
        }
    }
    return ret;
}

MKEY_state_e MKEYS_vHandleTask(void) {
    uint8_t ucPress;
    switch(mkey_state) {
        case MKEY_STATE_IDLE:
            break;
        case MKEY_STATE_NO_PRESS:
            mkey_state = MKEY_STATE_IDLE;
            mkey_ucTimerNoPress = MKEY_TIME_NO_PRESS;
            MKEY_vSetRowsState(MKEY_PINS_ROW_STATE_0);
            MKEY_vEnableInterrupts();
            break;
        case MKEY_STATE_SCAN:
            if(MKEY_bGetScan(0x0u, MKEY_PINS_ROW_STATE_1, &ucPress) ||
                    MKEY_bGetScan(0x1u, MKEY_PINS_ROW_STATE_2, &ucPress) ||
                    MKEY_bGetScan(0x2u, MKEY_PINS_ROW_STATE_3, &ucPress) ||
                    MKEY_bGetScan(0x3u, MKEY_PINS_ROW_STATE_4, &ucPress)) {
                mkey_ucTimerNoPress = MKEY_TIME_NO_PRESS;
                if(ucPress == mkey_ucPressLast) {                    
                    if(!mkey_ucTimer--) {
                        mkey_ucTimer = MKEY_TIME_PRESS;
                        mkey_ucTimerNoPress = MKEY_TIME_NO_PRESS;
                        mkey_state = MKEY_STATE_PRESS;
                        mkey_ucPress = mkey_ucPressLast;
                        mkey_bIsPress = true;
                    }
                } else {
                    mkey_ucPressLast = ucPress;
                    mkey_ucTimer = MKEY_TIME_PRESS;
                }
            } else {
                if(!mkey_ucTimerNoPress--) {
                    mkey_ucTimerNoPress = MKEY_TIME_NO_PRESS;
                    mkey_state = MKEY_STATE_NO_PRESS;
                }
            }
            break;
        case MKEY_STATE_PRESS:
            if(MKEY_bGetScan(0x0u, MKEY_PINS_ROW_STATE_1, &ucPress) ||
                    MKEY_bGetScan(0x1u, MKEY_PINS_ROW_STATE_2, &ucPress) ||
                    MKEY_bGetScan(0x2u, MKEY_PINS_ROW_STATE_3, &ucPress) ||
                    MKEY_bGetScan(0x3u, MKEY_PINS_ROW_STATE_4, &ucPress)) {
                mkey_ucTimerNoPress = MKEY_TIME_NO_PRESS;
            } else {
                // while to not button is press
                if(!mkey_ucTimerNoPress--) {
                    mkey_state = MKEY_STATE_NO_PRESS;
                }
            }
            break;
        case MKEY_STATE_NOT_INIT:
            mkey_bIsPress = false;
            mkey_ucPress = 0;
            mkey_ucPressLast = 0xFFu;
            mkey_ucTimer = MKEY_TIME_PRESS;
            mkey_ucTimerNoPress = MKEY_TIME_NO_PRESS;
            MKEY_vInitPort();
            mkey_state = MKEY_STATE_IDLE;
            MKEY_vInitExtInterrupts();
            break;
        case MKEY_STATE_ERROR:
            mkey_state = MKEY_STATE_IDLE;
            mkey_bIsPress = false;
            mkey_ucTimer = MKEY_TIME_PRESS;
            mkey_ucTimerNoPress = MKEY_TIME_NO_PRESS;
            MKEY_vSetRowsState(MKEY_PINS_ROW_STATE_0);
            MKEY_vEnableInterrupts();
            break;
    }
    return mkey_state;
}

void MKEY_vIRQHandler(void) {
    mkey_state = MKEY_STATE_SCAN;
    EXTI->PR1 |= MKEY_EXTI_PINS_Msk;
    MKEY_vDisableInterrupts();
}
#if((C0_PIN == 0) || (C1_PIN == 0) || (C2_PIN == 0) || (C3_PIN == 0))
void EXTI0_IRQHandler(void) {
    MKEY_vIRQHandler();
}
#endif
#if((C0_PIN == 1) || (C1_PIN == 1) || (C2_PIN == 1) || (C3_PIN == 1))
void EXTI1_IRQHandler(void) {
    MKEY_vIRQHandler();
}
#endif
#if((C0_PIN == 2) || (C1_PIN == 2) || (C2_PIN == 2) || (C3_PIN == 2))
void EXTI2_IRQHandler(void) {
    MKEY_vIRQHandler();
}
#endif
#if((C0_PIN == 3) || (C1_PIN == 3) || (C2_PIN == 3) || (C3_PIN == 3))
void EXTI3_IRQHandler(void) {
    MKEY_vIRQHandler();
}
#endif
#if((C0_PIN == 4) || (C1_PIN == 4) || (C2_PIN == 4) || (C3_PIN == 4))
void EXTI4_IRQHandler(void) {
    MKEY_vIRQHandler();
}
#endif
#if(((4u < C0_PIN) && (C0_PIN << 10u)) || ((4u < C1_PIN) && (C1_PIN << 10u)) || ((4u < C2_PIN) && (C2_PIN << 10u)) || ((4u < C3_PIN) && (C3_PIN << 10u)))
void EXTI9_5_IRQHandler(void) {
    MKEY_vIRQHandler();
}
#endif
#if(((9u < C0_PIN) && (C0_PIN << 16u)) || ((9u < C1_PIN) && (C1_PIN << 16u)) || ((9u < C2_PIN) && (C2_PIN << 16u)) || ((9u < C3_PIN) && (C3_PIN << 16u)))
void EXTI15_10_IRQHandler(void) {
    MKEY_vIRQHandler();
}
#endif
