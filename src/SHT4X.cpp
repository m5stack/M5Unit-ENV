#include "SHT4X.h"

bool SHT4X::begin(TwoWire* wire, uint8_t addr, uint8_t sda, uint8_t scl,
                  long freq) {
    _i2c.begin(wire, sda, scl, freq);
    _addr = addr;
    _wire = wire;
    return _i2c.exist(_addr);
}

bool SHT4X::update() {
    uint8_t readbuffer[6];
    uint8_t cmd       = SHT4x_NOHEAT_HIGHPRECISION;
    uint16_t duration = 10;

    if (_heater == SHT4X_NO_HEATER) {
        if (_precision == SHT4X_HIGH_PRECISION) {
            cmd      = SHT4x_NOHEAT_HIGHPRECISION;
            duration = 10;
        }
        if (_precision == SHT4X_MED_PRECISION) {
            cmd      = SHT4x_NOHEAT_MEDPRECISION;
            duration = 5;
        }
        if (_precision == SHT4X_LOW_PRECISION) {
            cmd      = SHT4x_NOHEAT_LOWPRECISION;
            duration = 2;
        }
    }

    if (_heater == SHT4X_HIGH_HEATER_1S) {
        cmd      = SHT4x_HIGHHEAT_1S;
        duration = 1100;
    }
    if (_heater == SHT4X_HIGH_HEATER_100MS) {
        cmd      = SHT4x_HIGHHEAT_100MS;
        duration = 110;
    }

    if (_heater == SHT4X_MED_HEATER_1S) {
        cmd      = SHT4x_MEDHEAT_1S;
        duration = 1100;
    }
    if (_heater == SHT4X_MED_HEATER_100MS) {
        cmd      = SHT4x_MEDHEAT_100MS;
        duration = 110;
    }

    if (_heater == SHT4X_LOW_HEATER_1S) {
        cmd      = SHT4x_LOWHEAT_1S;
        duration = 1100;
    }
    if (_heater == SHT4X_LOW_HEATER_100MS) {
        cmd      = SHT4x_LOWHEAT_100MS;
        duration = 110;
    }

    _i2c.writeByte(_addr, cmd, 1);

    delay(duration);

    _wire->requestFrom(_addr, (uint8_t)6);

    Serial.println();
    for (uint16_t i = 0; i < 6; i++) {
        readbuffer[i] = _wire->read();
    }

    if (readbuffer[2] != crc8(readbuffer, 2) ||
        readbuffer[5] != crc8(readbuffer + 3, 2)) {
        return false;
    }

    float t_ticks  = (uint16_t)readbuffer[0] * 256 + (uint16_t)readbuffer[1];
    float rh_ticks = (uint16_t)readbuffer[3] * 256 + (uint16_t)readbuffer[4];

    cTemp    = -45 + 175 * t_ticks / 65535;
    humidity = -6 + 125 * rh_ticks / 65535;
    humidity = min(max(humidity, (float)0.0), (float)100.0);
    return true;
}

void SHT4X::setPrecision(sht4x_precision_t prec) {
    _precision = prec;
}

sht4x_precision_t SHT4X::getPrecision(void) {
    return _precision;
}

void SHT4X::setHeater(sht4x_heater_t heat) {
    _heater = heat;
}

sht4x_heater_t SHT4X::getHeater(void) {
    return _heater;
}
