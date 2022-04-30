#pragma once
#include <Wire.h>

enum LDC2112Status:uint8_t
{
    LDC2112_STATUS_REGISTER_FLAG = 1 << 0,
    LDC2112_STATUS_TIMEOUT = 1 << 1,
    LDC2112_STATUS_LC_WD = 1 << 2,
    LDC2112_STATUS_FSM_WD = 1 << 3,
    LDC2112_STATUS_MAXOUT = 1 << 4,
    LDC2112_STATUS_RDY_TO_WRITE = 1 << 5,

    /**
     * @brief Chip Ready Status.
     * 
     */
    LDC2112_STATUS_CHIP_READY = 1 << 6,

    /**
     * @brief Output Status.
     * 
     * Logic OR of output bits from Register OUT. 
     * This field is cleared by reading this register.
     */
    LDC2112_STATUS_OUT_STATUS = 1 << 7,

    LDC2112_STATUS_ERROR = LDC2112_STATUS_REGISTER_FLAG 
        | LDC2112_STATUS_TIMEOUT 
        | LDC2112_STATUS_LC_WD 
        | LDC2112_STATUS_FSM_WD
};

enum LDC2112Out:uint8_t {
    LDC2112_OUT0 = 1 << 0,
    LDC2112_OUT1 = 1 << 1
};

struct LDC2112State
{
    uint8_t status;
    uint8_t out;
    int16_t data0;
    int16_t data1;
};

void ldc2112_begin(TwoWire *wire);
void ldc2112_config();
void ldc2112_read(LDC2112State *state);