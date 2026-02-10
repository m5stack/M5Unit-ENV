/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitENVIII
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <M5HAL.hpp>  // For NessoN1

// Using single shot measurement If defined
// #define USING_SINGLE_SHOT

// Using combined unit if defined
#define USING_ENV3

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

#if defined(USING_ENV3)
#pragma message "Using combined unit(ENV3)"
m5::unit::UnitENV3 unitENV3;
#else
#pragma message "Using each unit"
m5::unit::UnitSHT30 unitSHT30;
m5::unit::UnitQMP6988 unitQMP6988;
#endif

#if defined(USING_ENV3)
auto& sht30   = unitENV3.sht30;
auto& qmp6988 = unitENV3.qmp6988;
#else
auto& sht30   = unitSHT30;
auto& qmp6988 = unitQMP6988;
#endif
}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

#if defined(USING_SINGLE_SHOT)
    {
        auto cfg           = sht30.config();
        cfg.start_periodic = false;
        sht30.config(cfg);
    }
    {
        auto cfg           = qmp6988.config();
        cfg.start_periodic = false;
        qmp6988.config(cfg);
    }
#endif

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
#if defined(USING_ENV3)
        if (!Units.add(unitENV3, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
#else
        if (!Units.add(unitSHT30, i2c_bus ? i2c_bus.value() : nullptr) ||
            !Units.add(unitQMP6988, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
#endif
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
#if defined(USING_ENV3)
        if (!Units.add(unitENV3, Wire) || !Units.begin()) {
#else
        if (!Units.add(unitSHT30, Wire) || !Units.add(unitQMP6988, Wire) || !Units.begin()) {
#endif
            M5_LOGE("Failed to begin");
            M5_LOGW("%s", Units.debugInfo().c_str());
            lcd.clear(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
#if defined(USING_SINGLE_SHOT)
    M5_LOGI("\n>>> Click BtnA to single shot measurement");
#endif
    lcd.clear(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();

#if defined(USING_SINGLE_SHOT)
    if (M5.BtnA.wasClicked()) {
        m5::unit::sht30::Data ds{};
        if (sht30.measureSingleshot(ds)) {
            M5.Log.printf(">SHT30Temp:%2.2f\n>Humidity:%2.2f\n", ds.temperature(), ds.humidity());
        }
        m5::unit::qmp6988::Data dq{};
        if (qmp6988.measureSingleshot(dq)) {
            M5.Log.printf(">QMP6988Temp:%2.2f\n>Pressure:%.2f\n", dq.temperature(), dq.pressure() * 0.01f);
        }
    }
#else
    if (sht30.updated()) {
        M5.Log.printf(">SHT30Temp:%2.2f\n>Humidity:%2.2f\n", sht30.temperature(), sht30.humidity());
        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 3, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("SHT30\nTemp:%2.2f\nHumidity:%2.2f", sht30.temperature(), sht30.humidity());
        lcd.endWrite();
    }
    if (qmp6988.updated()) {
        M5.Log.printf(">QMP6988Temp:%2.2f\n>Pressure:%.2f\n", qmp6988.temperature(), qmp6988.pressure() * 0.01f);
        lcd.startWrite();
        lcd.fillRect(0, lcd.fontHeight() * 3, lcd.width(), lcd.fontHeight() * 3, TFT_BLACK);
        lcd.setCursor(0, lcd.fontHeight() * 3);
        lcd.printf("QMP6988\nTemp:%2.2f\nPressure:%.2f", qmp6988.temperature(), qmp6988.pressure() * 0.01f);
        lcd.endWrite();
    }
#endif
}
