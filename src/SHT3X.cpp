#include "SHT3X.h"

bool SHT3X::begin(TwoWire* wire, uint8_t addr, uint8_t sda, uint8_t scl,
                  long freq) {
    _i2c.begin(wire, sda, scl, freq);
    _addr = addr;
    _wire = wire;
    return _i2c.exist(_addr);
}

bool SHT3X::update() {
    unsigned int data[6];

    // Start I2C Transmission
    _wire->beginTransmission(_addr);
    // Send measurement command
    _wire->write(0x2C);
    _wire->write(0x06);
    // Stop I2C transmission
    if (_wire->endTransmission() != 0) return false;

    delay(200);

    // Request 6 bytes of data
    _wire->requestFrom(_addr, (uint8_t)6);

    // Read 6 bytes of data
    // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
    for (int i = 0; i < 6; i++) {
        data[i] = _wire->read();
    };

    delay(50);

    if (_wire->available() != 0) return false;

    // Convert the data
    cTemp    = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
    fTemp    = (cTemp * 1.8) + 32;
    humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);

    return true;
}
