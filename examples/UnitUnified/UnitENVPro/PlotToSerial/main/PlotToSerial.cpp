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
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware I2C wiring helpers

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitENVPro unit;
}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
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
    if (unit.updated()) {
#if defined(UNIT_BME688_USING_BSEC2)
        M5.Log.printf(">IAQ:%.2f\n>Temperature:%.2f\n>Pressure:%.2f\n>Humidity:%.2f\n>GAS:%.2f\n>Altitude:%.2f\n",
                      unit.iaq(), unit.temperature(), unit.pressure(), unit.humidity(), unit.gas(), unit.altitude());
        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 6, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("IAQ:%.2f\nTemperature:%.2f\nPressure:%.2f\nHumidity:%.2f\nGAS:%.2f\nAltitude:%.2f", unit.iaq(),
                   unit.temperature(), unit.pressure(), unit.humidity(), unit.gas(), unit.altitude());
        lcd.endWrite();
#else
        M5.Log.printf(">Temperature:%.2f\n>Pressure:%.2f\n>Humidity:%.2f\n>GAS:%.2f\n>Altitude:%.2f\n",
                      unit.temperature(), unit.pressure(), unit.humidity(), unit.gas(), unit.altitude());
        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 5, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("Temperature:%.2f\nPressure:%.2f\nHumidity:%.2f\nGAS:%.2f\nAltitude:%.2f", unit.temperature(),
                   unit.pressure(), unit.humidity(), unit.gas(), unit.altitude());
        lcd.endWrite();
        // m5::utility::delay(1000);
#endif
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
