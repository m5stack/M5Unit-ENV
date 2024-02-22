
#ifndef _DHT12_H
#define _DHT12_H

#include "Arduino.h"
#include "Wire.h"
#include "I2C_Class.h"

#define DHT12_I2C_ADDR 0x5c

class DHT12 {
   public:
    bool begin(TwoWire* wire = &Wire, uint8_t addr = DHT12_I2C_ADDR,
               uint8_t sda = 21, uint8_t scl = 22, long freq = 400000U);
    bool update();
    float cTemp    = 0;
    float fTemp    = 0;
    float kTemp    = 0;
    float humidity = 0;

   private:
    uint8_t _addr;
    I2C_Class _i2c;
};

#endif
