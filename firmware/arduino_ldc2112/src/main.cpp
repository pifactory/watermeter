#include <Arduino.h>
#include <Wire.h>
#include <arduino_ldc2112.h>

static uint16_t counter = 0;
static uint16_t errors = 0;
static uint8_t out_prev = 0;
static LDC2112Out lastInterrupt = LDC2112_OUT0;
static LDC2112State ldc2112_state;

static void IRAM_ATTR sensor_int0()
{
  counter++;
  lastInterrupt = LDC2112_OUT0;
}
static void IRAM_ATTR sensor_int1()
{
  if (lastInterrupt == LDC2112_OUT1)
  {
    // missed signal from the first channel
    errors++;
    counter++;
  }
  lastInterrupt = LDC2112_OUT1;
}

static bool isTriggered(LDC2112Out out_)
{
  bool prevState = out_prev & out_;
  bool currentState = ldc2112_state.out & out_;

  return !prevState && currentState;
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  ldc2112_begin(&Wire);
}

void loop()
{
  ldc2112_read(&ldc2112_state);
  if (isTriggered(LDC2112_OUT0))
  {
    sensor_int0();
  }
  if (isTriggered(LDC2112_OUT1))
  {
    sensor_int1();
  }
  out_prev = ldc2112_state.out;

  static uint_fast8_t i = 0;
  if (i == 0)
  {
    Serial.printf("state=%x out=%x counter=%u, errors=%u, data0=%d, data1=%d\n",
                  ldc2112_state.status,
                  ldc2112_state.out,
                  counter,
                  errors,
                  ldc2112_state.data0,
                  ldc2112_state.data1);
  }
  i = (i + 1) % 2;

  delay(500);
}