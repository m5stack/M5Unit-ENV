/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitCO2L
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <M5HAL.hpp>  // For NessoN1

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitCO2L unit;

}  // namespace

using namespace m5::unit::scd4x;

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
        unit_ready = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
        unit_ready = Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
        unit_ready = Units.add(unit, Wire) && Units.begin();
    }
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    {
        auto ret = unit.stopPeriodicMeasurement();
        float offset{};
        ret &= unit.readTemperatureOffset(offset);
        uint16_t altitude{};
        ret &= unit.readSensorAltitude(altitude);
        uint16_t pressure{};
        ret &= unit.readAmbientPressure(pressure);
        bool asc{};
        ret &= unit.readAutomaticSelfCalibrationEnabled(asc);
        uint16_t ppm{};
        ret &= unit.readAutomaticSelfCalibrationTarget(ppm);
        uint16_t initialPeriod{}, standardPeriod{};
        ret &= unit.readAutomaticSelfCalibrationInitialPeriod(initialPeriod);
        ret &= unit.readAutomaticSelfCalibrationStandardPeriod(standardPeriod);

        ret &= unit.startPeriodicMeasurement();

        M5.Log.printf(
            "     temp offset:%f\n"
            " sensor altitude:%u\n"
            "ambient pressure:%u\n"
            "     ASC enabled:%u\n"
            "      ASC target:%u\n"
            "  initial period:%u\n"
            " standard period:%u\n",
            offset, altitude, pressure, asc, ppm, initialPeriod, standardPeriod);

        if (!ret) {
            lcd.fillScreen(TFT_RED);
            while (true) {
                m5::utility::delay(1000);
            }
        }
    }
    lcd.fillScreen(TFT_DARKGREEN);
}

void loop()
{
    M5.update();

    // Periodic
    Units.update();
    if (unit.updated()) {
        // Can be checked e.g. by serial plotters
        M5.Log.printf(">CO2:%u\n>Temperature:%2.2f\n>Humidity:%2.2f\n", unit.co2(), unit.temperature(),
                      unit.humidity());
        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 3, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("CO2:%u\nTemperature:%2.2f\nHumidity:%2.2f", unit.co2(), unit.temperature(), unit.humidity());
        lcd.endWrite();
    }

    // Single shot
    if (M5.BtnA.wasClicked()) {
        static bool all{};  // false: RHT only
        all = !all;
        M5.Log.printf("Try single shot %u, waiting measurement...\n", all);
        unit.stopPeriodicMeasurement();
        Data d{};
        if (all) {
            if (unit.measureSingleshot(d)) {
                M5.Log.printf("  SingleAll: CO2:%u T:%2.2f H:%2.2f\n", d.co2(), d.temperature(), d.humidity());
            }
        } else {
            if (unit.measureSingleshotRHT(d)) {
                M5.Log.printf("  SingleRHT: T:%2.2f H:%2.2f\n", d.temperature(), d.humidity());
            }
        }
        unit.startPeriodicMeasurement();
    }
}
