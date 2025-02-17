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
// m5::unit::UnitCO2 unit;
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
        M5_LOGI("\n>CO2:%u\n>Temperature:%2.2f\n>Humidity:%2.2f", unit.co2(), unit.temperature(), unit.humidity());
    }

    // Single
    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
        static bool all{};  // false: RHT only
        all = !all;
        M5_LOGI("Try single shot %u, waiting measurement...", all);
        unit.stopPeriodicMeasurement();
        Data d{};
        if (all) {
            if (unit.measureSingleshot(d)) {
                M5_LOGI("SingleAll: %u/%2.2f/%2.2f", d.co2(), d.temperature(), d.humidity());
            }
        } else {
            if (unit.measureSingleshotRHT(d)) {
                M5_LOGI("SingleRHT: %2.2f/%2.2f", d.temperature(), d.humidity());
            }
        }
        unit.startPeriodicMeasurement();
    }
}
