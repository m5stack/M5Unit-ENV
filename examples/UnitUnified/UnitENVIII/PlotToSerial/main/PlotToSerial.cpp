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

    auto board = M5.getBoard();

    // NessoN1: Arduino Wire (I2C_NUM_0) cannot be used for GROVE port.
    //   Wire is used by M5Unified In_I2C for internal devices (IOExpander etc.).
    //   Solution: Use SoftwareI2C via M5HAL (bit-banging) for the GROVE port.
    // NanoC6: Wire.begin() on GROVE pins conflicts with m5::I2C_Class registered by Ex_I2C.setPort()
    //   on the same I2C_NUM_0, causing sporadic NACK errors.
    //   Solution: Use M5.Ex_I2C (m5::I2C_Class) directly instead of Arduino Wire.
    bool unit_ready{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a (which maps to Wire pins 8/10)
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
#if defined(USING_ENV3)
        unit_ready = Units.add(unitENV3, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
#else
        unit_ready = Units.add(unitSHT30, i2c_bus ? i2c_bus.value() : nullptr) &&
                     Units.add(unitQMP6988, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
#endif
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
#if defined(USING_ENV3)
        unit_ready = Units.add(unitENV3, M5.Ex_I2C) && Units.begin();
#else
        unit_ready = Units.add(unitSHT30, M5.Ex_I2C) && Units.add(unitQMP6988, M5.Ex_I2C) && Units.begin();
#endif
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
#if defined(USING_ENV3)
        unit_ready = Units.add(unitENV3, Wire) && Units.begin();
#else
        unit_ready = Units.add(unitSHT30, Wire) && Units.add(unitQMP6988, Wire) && Units.begin();
#endif
    }
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.fillScreen(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();

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

    if (M5.BtnA.wasClicked()) {
        sht30.stopPeriodicMeasurement();
        qmp6988.stopPeriodicMeasurement();

        m5::unit::sht30::Data ds{};
        if (sht30.measureSingleshot(ds)) {
            M5.Log.printf("== Singleshot SHT30 Temp:%2.2f Humidity:%2.2f\n", ds.temperature(), ds.humidity());
        } else {
            M5_LOGW("Single SHT30 failed");
        }
        m5::unit::qmp6988::Data dq{};
        if (qmp6988.measureSingleshot(dq)) {
            M5.Log.printf("== Singleshot QMP6988 Temp:%2.2f Pressure:%.2f\n", dq.temperature(), dq.pressure() * 0.01f);
        } else {
            M5_LOGW("Single QMP failed");
        }

        sht30.startPeriodicMeasurement();
        qmp6988.startPeriodicMeasurement();
    }
}
