#include "DHT12.h"

bool DHT12::begin(TwoWire* wire, uint8_t addr, uint8_t sda, uint8_t scl,
                  long freq) {
    _i2c.begin(wire, sda, scl, freq);
    _addr = addr;
    return _i2c.exist(_addr);
}

bool DHT12::update() {
    uint8_t datos[5] = {0};
    if (_i2c.readBytes(_addr, 0, datos, 5)) {
        if (datos[4] != (datos[0] + datos[1] + datos[2] + datos[3])) {
            return false;
        }
        cTemp    = (datos[2] + (float)datos[3] / 10);
        fTemp    = ((datos[2] + (float)datos[3] / 10) * 1.8 + 32);
        kTemp    = (datos[2] + (float)datos[3] / 10) + 273.15;
        humidity = (datos[0] + (float)datos[1] / 10);
        return true;
    }
    return false;
}
