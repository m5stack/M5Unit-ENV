/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitENVIV
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>

//#define USING_ENV4

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

#if defined(USING_ENV4)
#pragma message "Using combined unit(ENV4)"
m5::unit::UnitENV4 unitENV4;
#else
#pragma message "Using each unit"
m5::unit::UnitSHT40 unitSHT40;
//m5::unit::UnitQMP6988 unitQMP6988;
#endif

#if defined(USING_ENV4)
auto& sht40 = unitENV4.sht40;
// auto& qmp6988 = unitENV4.qmp6988;
#else
auto& sht40 = unitSHT40;
// auto& qmp6988 = unitQMP6988;
#endif
}  // namespace

void setup()
{
    M5.begin();

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

    /*
    {
        auto cfg                     = qmp6988.config();
        cfg.oversampling_temperature = m5::unit::qmp6988::Oversampling::X1;
        cfg.oversampling_pressure    = m5::unit::qmp6988::Oversampling::X1;
        cfg.filter                   = m5::unit::qmp6988::Filter::Coeff16;
        cfg.standby_time             = m5::unit::qmp6988::Standby::Time1ms;
        qmp6988.config(cfg);
    }
    */

#if defined(USING_ENV4)
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);

    if (!Units.add(unitENV4, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#else
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);
    //    if (!Units.add(unitSHT40, Wire) || !Units.add(unitQMP6988, Wire) || !Units.begin()) {
    if (!Units.add(unitSHT40, Wire) || !Units.begin()) {
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
    Units.update();

    if (sht40.updated()) {
        M5_LOGI("\n>SHT40Temp:%2.2f\n>Humidity:%2.2f", sht40.temperature(), sht40.humidity());
    }
    /*
    if (qmp6988.updated()) {
        M5_LOGI("\n>QMP6988Temp:%2.2f\n>Pressure:%.2f", qmp6988.temperature(), qmp6988.pressure());
    }
    */
}
