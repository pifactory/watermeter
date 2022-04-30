#include "arduino_ldc2112.h"

static constexpr uint8_t LDC2112_I2C_ADDR = 0x2A;

// Low power enable channel 0
static constexpr uint8_t LPEN0 = 0x10;
// Low power enable channel 1
static constexpr uint8_t LPEN1 = 0x20;
// Active high for OUT0-1, default data polarity
static constexpr uint8_t OPOL_ACTIVE_HIGH = 0x3F;
// Frequency range between 3.3MHz and 10MHz
static constexpr uint8_t SENSOR_CONFIG_FREQ33 = 0x20;

// Exponent for tracking baseline increment in LP mode, 7 is the maximum
static constexpr uint8_t LB_BASE_INC_FAST = 5;
static constexpr uint8_t LB_BASE_INC_SLOW = 4;

// Pause baseline tracking for ch0 when OUT0=1
static constexpr uint8_t BTPAUSE0 = 0x10;
// Pause baseline tracking for ch1 when OUT1=1
static constexpr uint8_t BTPAUSE1 = 0x20;
static constexpr uint8_t BTPAUSE_ALL = BTPAUSE0 | BTPAUSE1;

// CNTSC=0 for channel 0
static constexpr uint8_t CNTSC0_0 = 0b0000;
// CNTSC=1 for channel 0
static constexpr uint8_t CNTSC0_1 = 0b0001;
// CNTSC=2 for channel 0
static constexpr uint8_t CNTSC0_2 = 0b0010;
// CNTSC=3 for channel 0
static constexpr uint8_t CNTSC0_3 = 0b0011;
// CNTSC=0 for channel 1
static constexpr uint8_t CNTSC1_0 = 0b0000;
// CNTSC=1 for channel 1
static constexpr uint8_t CNTSC1_1 = 0b0100;
// CNTSC=2 for channel 1
static constexpr uint8_t CNTSC1_2 = 0b1000;
// CNTSC=3 for channel 1
static constexpr uint8_t CNTSC1_3 = 0b1100;

static constexpr uint8_t REG_CNTSC = 0x1E;
static constexpr uint8_t REG_EN = 0x0C;
static constexpr uint8_t REG_GAIN0 = 0x0E;
static constexpr uint8_t REG_GAIN1 = 0x10;
static constexpr uint8_t REG_LP_BASE_INC = 0x13;
static constexpr uint8_t REG_OPOL_DPOL = 0x1C;
static constexpr uint8_t REG_SENSOR0_CONFIG = 0x20;
static constexpr uint8_t REG_SENSOR1_CONFIG = 0x22;
static constexpr uint8_t REG_BTPAUSE_MAXWIN = 0x16;

static TwoWire *ldc2112_wire{nullptr};

static void i2cDeviceWriteReg8(uint8_t addr, uint8_t reg, uint8_t data);
static uint8_t i2cDeviceReadReg8(uint8_t addr, uint8_t reg);
static int16_t ldc2112_read_data(uint8_t data_reg);
static uint16_t i2cDeviceReadReg16LsbFirst(uint8_t addr, uint8_t reg);

void ldc2112_begin(TwoWire *wire)
{
    //   attachInterrupt();
    ldc2112_wire = wire;
    ldc2112_config();
}

void ldc2112_read(LDC2112State *state)
{
    state->status = i2cDeviceReadReg8(LDC2112_I2C_ADDR, 0x00);
    state->out = i2cDeviceReadReg8(LDC2112_I2C_ADDR, 0x01);
    state->data0 = ldc2112_read_data(0x02);
    state->data1 = ldc2112_read_data(0x04);
}

void ldc2112_config()
{
    // Start configuration
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, 0x0A, 1);

    // Enable channel 0 and 1 in low-power mode
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_EN, LPEN0 | LPEN1);
    // Set active high on outputs 0-1 (Default: active low)
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_OPOL_DPOL, OPOL_ACTIVE_HIGH);

    // Set scan rate in LP mode to 1.25SPS
    // 0x03 = 0.625 SPS
    // 0x02 = 1.25 SPS (Default)
    // 0x01 = 2.5 SPS
    // 0x00 = 5 SPS
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, 0x0F, 0x02);
    // Set LCDIV
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, 0x17, 0x03);
    // Set frequency between 3.3 and 10Mhz, SENCYC=5
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_SENSOR0_CONFIG, SENSOR_CONFIG_FREQ33 | 5);
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_SENSOR1_CONFIG, SENSOR_CONFIG_FREQ33 | 5);
    // Set CNTSC0=0, CNTSC1=0
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_CNTSC, CNTSC0_0 | CNTSC1_0);

    // Set gain, default = 40 (x32).
    // High gain reduces blind spot, but can make reading less reliable (???)
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_GAIN0, 32);
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_GAIN1, 32);

    // Disable baseline tracking when asserted
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_BTPAUSE_MAXWIN, BTPAUSE_ALL);
    // Change baseline with 2^exp/9 per scan cycle in LP mode, default = 5
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, REG_LP_BASE_INC, LB_BASE_INC_SLOW);
    // Set x8 faster baseline tracking when data is negative
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, 0x25, 0x06);
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, 0x28, 0x70);

    // End configuration
    i2cDeviceWriteReg8(LDC2112_I2C_ADDR, 0x0A, 0);
}

static void i2cDeviceWriteReg8(uint8_t addr, uint8_t reg, uint8_t data)
{
    ldc2112_wire->beginTransmission(addr);
    ldc2112_wire->write(reg);
    ldc2112_wire->write(data);
    ldc2112_wire->endTransmission();
}

static uint8_t i2cDeviceReadReg8(uint8_t addr, uint8_t reg)
{
    ldc2112_wire->beginTransmission(addr);
    ldc2112_wire->write(reg);
    ldc2112_wire->endTransmission();
    ldc2112_wire->requestFrom(addr, 1U);
    return ldc2112_wire->read();
}

static int16_t ldc2112_read_data(uint8_t data_reg)
{
    auto data = i2cDeviceReadReg16LsbFirst(LDC2112_I2C_ADDR, data_reg);
    if (data & 0x800)
    {
        data = data | 0xf000;
    }
    return data;
}

static uint16_t i2cDeviceReadReg16LsbFirst(uint8_t addr, uint8_t reg)
{
    ldc2112_wire->beginTransmission(addr);
    ldc2112_wire->write(reg);
    ldc2112_wire->endTransmission();
    ldc2112_wire->requestFrom(addr, 2U);
    uint16_t val = 0;
    val = ldc2112_wire->read();
    val = val | (ldc2112_wire->read() << 8);
    return val;
}