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

// #define USING_M5HAL      // When using M5HAL

// Using single shot measurement If defined
// #define USING_SINGLE_SHOT

// Using combined unit if defined
// #define USING_ENV3

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

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

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

    {
        auto cfg                     = qmp6988.config();
        cfg.oversampling_temperature = m5::unit::qmp6988::Oversampling::X1;
        cfg.oversampling_pressure    = m5::unit::qmp6988::Oversampling::X1;
        cfg.filter                   = m5::unit::qmp6988::Filter::Coeff16;
        cfg.standby_time             = m5::unit::qmp6988::Standby::Time1ms;
        qmp6988.config(cfg);
    }

#if defined(USING_ENV3)
#if defined(USING_M5HAL)
#pragma message "Using M5HAL"
    m5::hal::bus::I2CBusConfig i2c_cfg;
    i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
    i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
    auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
    if (!Units.add(unitENV3, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#else
#pragma message "Using Wire"
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);

    if (!Units.add(unitENV3, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#endif

#else

#if defined(USING_M5HAL)
#pragma message "Using M5HAL"
    m5::hal::bus::I2CBusConfig i2c_cfg;
    i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
    i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
    auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
    if (!Units.add(unitSHT30, i2c_bus ? i2c_bus.value() : nullptr) ||
        !Units.add(unitQMP6988, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#else
#pragma message "Using Wire"
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);
    if (!Units.add(unitSHT30, Wire) || !Units.add(unitQMP6988, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#endif
#endif

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
            M5_LOGI("\n>SHT30Temp:%2.2f\n>Humidity:%2.2f", ds.temperature(), ds.humidity());
        }
        m5::unit::qmp6988::Data dq{};
        if (qmp6988.measureSingleshot(dq)) {
            M5_LOGI("\n>QMP6988Temp:%2.2f\n>Pressure:%.2f", dq.temperature(), dq.pressure());
        }
    }
#else
    if (sht30.updated()) {
        M5_LOGI("\n>SHT30Temp:%2.2f\n>Humidity:%2.2f", sht30.temperature(), sht30.humidity());
    }
    if (qmp6988.updated()) {
        M5_LOGI("\n>QMP6988Temp:%2.2f\n>Pressure:%.2f", qmp6988.temperature(), qmp6988.pressure());
    }
#endif
}
