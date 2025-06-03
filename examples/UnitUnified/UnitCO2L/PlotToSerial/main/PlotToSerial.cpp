/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitCO2L
*/
// #define USING_M5HAL  // When using M5HAL
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitCO2L unit;

}  // namespace

using namespace m5::unit::scd4x;

void setup()
{
    M5.begin();

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

#if defined(USING_M5HAL)
#pragma message "Using M5HAL"
    // Using M5HAL
    m5::hal::bus::I2CBusConfig i2c_cfg;
    i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
    i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
    auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
    if (!Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#else
#pragma message "Using Wire"
    // Using TwoWire
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);
    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#endif
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
            lcd.clear(TFT_RED);
            while (true) {
                m5::utility::delay(1000);
            }
        }
    }
    lcd.clear(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();

    // Periodic
    Units.update();
    if (unit.updated()) {
        // Can be checked e.g. by serial plotters
        M5.Log.printf(">CO2:%u\n>Temperature:%2.2f\n>Humidity:%2.2f\n", unit.co2(), unit.temperature(),
                      unit.humidity());
    }

    // Single shot
    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
        static bool all{};  // false: RHT only
        all = !all;
        M5.Log.printf("Try single shot %u, waiting measurement...\n", all);
        unit.stopPeriodicMeasurement();
        Data d{};
        if (all) {
            if (unit.measureSingleshot(d)) {
                M5.Log.printf("   SingleAll: %u/%2.2f/%2.2f\n", d.co2(), d.temperature(), d.humidity());
            }
        } else {
            if (unit.measureSingleshotRHT(d)) {
                M5.Log.printf("  SingleRHT: %2.2f/%2.2f", d.temperature(), d.humidity());
            }
        }
        unit.startPeriodicMeasurement();
    }
}
