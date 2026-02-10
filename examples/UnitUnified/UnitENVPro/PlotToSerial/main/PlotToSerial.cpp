/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitENVPro
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <M5HAL.hpp>  // For NessoN1

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitENVPro unit;
}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    // For NessoN1 GROVE
    if (M5.getBoard() == m5::board_t::board_ArduinoNessoN1) {
        // Port A of the NessoN1 is QWIIC, then use portB (GROVE)
        pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(NessoN1): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        // Wire is used internally, so SoftwareI2C handles the unit
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        if (!Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.clear(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    } else {
        // Using TwoWire
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
        if (!Units.add(unit, Wire) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.clear(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.clear(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();
    if (unit.updated()) {
#if defined(UNIT_BME688_USING_BSEC2)
        M5.Log.printf(">IAQ:%.2f\n>Temperature:%.2f\n>Pressure:%.2f\n>Humidity:%.2f\n>GAS:%.2f\n", unit.iaq(),
                      unit.temperature(), unit.pressure(), unit.humidity(), unit.gas());
        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 5, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("IAQ:%.2f\nTemperature:%.2f\nPressure:%.2f\nHumidity:%.2f\nGAS:%.2f", unit.iaq(), unit.temperature(),
                   unit.pressure(), unit.humidity(), unit.gas());
        lcd.endWrite();
#else
        M5.Log.printf(">Temperature:%.2f\n>Pressure:%.2f\n>Humidity:%.2f\n>GAS:%.2f\n", unit.temperature(),
                      unit.pressure(), unit.humidity(), unit.gas());
        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 4, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("Temperature:%.2f\nPressure:%.2f\nHumidity:%.2f\nGAS:%.2f", unit.temperature(), unit.pressure(),
                   unit.humidity(), unit.gas());
        lcd.endWrite();
        // m5::utility::delay(1000);
#endif
    }
}
