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
#include <cmath>

#define USING_ENV4

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

#if defined(USING_ENV4)
#pragma message "Using combined unit(ENV4)"
m5::unit::UnitENV4 unitENV4;
#else
#pragma message "Using each unit"
m5::unit::UnitSHT40 unitSHT40;
m5::unit::UnitBMP280 unitBMP280;
#endif

#if defined(USING_ENV4)
auto& sht40  = unitENV4.sht40;
auto& bmp280 = unitENV4.bmp280;
#else
auto& sht40  = unitSHT40;
auto& bmp280 = unitBMP280;
#endif

float calculate_altitude(const float pressure, const float seaLvhPa = 1013.25f)
{
    return 44330.f * (1.0f - pow((pressure / 100.f) / seaLvhPa, 0.1903f));
}

}  // namespace

void setup()
{
    M5.begin();

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);

    {
        using namespace m5::unit::bmp280;
        auto cfg             = bmp280.config();
        cfg.osrs_pressure    = Oversampling::X16;
        cfg.osrs_temperature = Oversampling::X2;
        cfg.filter           = Filter::Coeff16;
        cfg.standby          = Standby::Time500ms;
        bmp280.config(cfg);
    }

#if defined(USING_ENV4)
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);

    if (!Units.add(unitENV4, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
#else
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);
    if (!Units.add(unitSHT40, Wire) || !Units.add(unitBMP280, Wire) || !Units.begin()) {
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
        M5.Log.printf(
            ">SHT40Temp:%.4f\n"
            ">Humidity:%.4f\n",
            sht40.temperature(), sht40.humidity());
    }
    if (bmp280.updated()) {
        auto p = bmp280.pressure();
        M5.Log.printf(
            ">BMP280Temp:%.4f\n"
            ">Pressure:%.4f\n"
            ">Altitude:%.4f\n",
            bmp280.temperature(), p * 0.01f /* To hPa */, calculate_altitude(p));
    }
}
