#ifndef __SHT3X_H_
#define __SHT3X_H_

#include "Arduino.h"
#include "I2C_Class.h"
#include "Wire.h"

#define SHT3X_I2C_ADDR 0x44

class SHT3X {
   public:
    bool begin(TwoWire* wire = &Wire, uint8_t addr = SHT3X_I2C_ADDR,
               uint8_t sda = 21, uint8_t scl = 22, long freq = 400000U);
    bool update(void);
    float cTemp    = 0;
    float fTemp    = 0;
    float humidity = 0;

   private:
    TwoWire* _wire;
    uint8_t _addr;
    I2C_Class _i2c;
};

#endif
