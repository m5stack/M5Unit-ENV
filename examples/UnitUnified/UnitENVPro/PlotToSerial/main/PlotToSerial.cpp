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

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitENVPro unit;
}  // namespace

void setup()
{
    m5::utility::delay(2000);

    M5.begin();

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
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
        M5_LOGI("\n>IAQ:%.2f\n>Temperature:%.2f\n>Pressure:%.2f\n>Humidity:%.2f\n>GAS:%.2f", unit.iaq(),
                unit.temperature(), unit.pressure(), unit.humidity(), unit.gas());
#else
        M5_LOGI("\n>Temperature:%.2f\n>Pressure:%.2f\n>Humidity:%.2f\n>GAS:%.2f", unit.temperature(), unit.pressure(),
                unit.humidity(), unit.gas());
        m5::utility::delay(1000);
#endif
    }
}
