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
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware I2C wiring helpers

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitENV4 unit;
auto& sht40  = unit.sht40;
auto& bmp280 = unit.bmp280;
}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    {
        using namespace m5::unit::bmp280;
        auto cfg             = bmp280.config();
        cfg.osrs_pressure    = Oversampling::X16;
        cfg.osrs_temperature = Oversampling::X2;
        cfg.filter           = Filter::Coeff16;
        cfg.standby          = Standby::Time500ms;
        bmp280.config(cfg);
    }

    // Board-aware I2C: NessoN1 -> SoftwareI2C(port_b), NanoC6/NanoH2 -> Ex_I2C, others -> Wire
    const bool unit_ready = m5::unit::wiring::addI2C(Units, unit) && Units.begin();
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.fillScreen(TFT_DARKGREEN);
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

        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 3, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf(
            "SHT40\n"
            "Temp:%.4f\n"
            "Humidity:%.4f",
            sht40.temperature(), sht40.humidity());
        lcd.endWrite();
    }
    if (bmp280.updated()) {
        M5.Log.printf(
            ">BMP280Temp:%.4f\n"
            ">Pressure:%.4f\n"
            ">Altitude:%.4f\n",
            bmp280.temperature(), bmp280.pressure() * 0.01f /* To hPa */, bmp280.altitude());

        lcd.startWrite();
        lcd.fillRect(0, lcd.fontHeight() * 3, lcd.width(), lcd.fontHeight() * 4, TFT_BLACK);
        lcd.setCursor(0, lcd.fontHeight() * 3);
        lcd.printf(
            "BMP280\n"
            "Temp:%.4f\n"
            "Pressure:%.4f\n"
            "Altitude:%.4f",
            bmp280.temperature(), bmp280.pressure() * 0.01f /* To hPa */, bmp280.altitude());
        lcd.endWrite();
    }

    if (M5.BtnA.wasClicked()) {
        sht40.stopPeriodicMeasurement();
        bmp280.stopPeriodicMeasurement();

        m5::unit::sht40::Data ds{};
        if (sht40.measureSingleshot(ds)) {
            M5.Log.printf("== Singleshot SHT40 Temp:%.4f Humidity:%.4f\n", ds.temperature(), ds.humidity());
        } else {
            M5_LOGW("Single SHT40 failed");
        }
        m5::unit::bmp280::Data db{};
        if (bmp280.measureSingleshot(db)) {
            M5.Log.printf("== Singleshot BMP280 Temp:%.4f Pressure:%.4f\n", db.temperature(), db.pressure() * 0.01f);
        } else {
            M5_LOGW("Single BMP280 failed");
        }

        sht40.startPeriodicMeasurement();
        bmp280.startPeriodicMeasurement();
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
