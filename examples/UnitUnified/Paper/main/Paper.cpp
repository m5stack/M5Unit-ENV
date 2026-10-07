/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of Using the M5UnitUnified for SHT30 Built into M5Paper
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // for failStop()
#include <cstdio>

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitSHT30 sht30;
float latest_temperature{};
float latest_humidity{};
bool has_measurement{};
uint32_t last_lcd_update_ms{};
constexpr uint32_t lcd_update_interval_ms{60 * 1000U};

void draw_dashboard(const float temperature, const float humidity, const bool force_full_refresh = false)
{
    char temp_text[32];
    char hum_text[32];
    std::snprintf(temp_text, sizeof(temp_text), "%7.2f C ", temperature);
    std::snprintf(hum_text, sizeof(hum_text), "%7.2f %% ", humidity);

    const int w = lcd.width();
    const int h = lcd.height();

    constexpr int value_x{220};
    constexpr int temp_value_y{130};
    constexpr uint8_t full_refresh_cycle{5};
    const int humid_value_y = h / 2 + 58;

    static bool initialized{};
    static uint8_t partial_count{};

    // Set EPD mode before any drawing to avoid mid-frame refresh
    if (force_full_refresh || ++partial_count >= full_refresh_cycle) {
        lcd.setEpdMode(m5gfx::epd_mode_t::epd_quality);
        partial_count = 0;
    } else {
        lcd.setEpdMode(m5gfx::epd_mode_t::epd_text);
    }

    if (!initialized || force_full_refresh) {
        lcd.fillScreen(TFT_WHITE);
        lcd.drawRoundRect(12, 12, w - 24, h - 24, 16, TFT_BLACK);
        lcd.drawFastHLine(28, 88, w - 56, TFT_BLACK);
        lcd.drawFastHLine(28, h / 2 + 20, w - 56, TFT_BLACK);

        lcd.setTextColor(TFT_BLACK, TFT_WHITE);
        lcd.setTextSize(1);
        lcd.setCursor(40, 28);
        lcd.print("M5PAPER SHT30 MONITOR");

        lcd.setCursor(40, 116);
        lcd.print("TEMP");
        lcd.setCursor(40, h / 2 + 44);
        lcd.print("HUMID");

        lcd.setTextSize(1);
        lcd.setCursor(w - 440, h - 44);
        lcd.print("refresh:touch or 60s");
        initialized = true;
    }

    lcd.waitDisplay();
    lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    lcd.setTextSize(3.5f);
    lcd.setTextPadding(w - value_x - 28);
    lcd.drawString(temp_text, value_x, temp_value_y);
    lcd.drawString(hum_text, value_x, humid_value_y);
    lcd.setTextPadding(0);
}
}  // namespace

void setup()
{
    {
        auto cfg = sht30.config();
        cfg.mps  = m5::unit::sht30::MPS::Half;  // 0.5Hz (about every 2 seconds)
        sht30.config(cfg);
    }

    M5.begin();
    if (M5.getBoard() != m5::board_t::board_M5Paper) {
        M5_LOGE("This example is for the SHT30 sensor built into the M5Paper");
        m5::unit::wiring::failStop();
    }

    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }
    lcd.setFont(&fonts::Orbitron_Light_32);

    // The built-in SHT30 is used via M5.In_I2C
    if (!Units.add(sht30, M5.In_I2C) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.fillScreen(TFT_WHITE);
    last_lcd_update_ms = m5::utility::millis() - 60 * 1000U;
}

void loop()
{
    M5.update();
    Units.update();
    const auto now = m5::utility::millis();
    bool touch_redraw{};

    if (M5.Touch.getCount()) {
        const auto td = M5.Touch.getDetail(0);
        touch_redraw  = td.wasPressed();
    }

    if (sht30.updated()) {
        latest_temperature = sht30.temperature();
        latest_humidity    = sht30.humidity();
        has_measurement    = true;
        M5.Log.printf(">SHT30Temp:%2.2f\n>Humidity:%2.2f\n", latest_temperature, latest_humidity);
    }

    if (has_measurement && (now - last_lcd_update_ms >= lcd_update_interval_ms || touch_redraw)) {
        lcd.startWrite();
        draw_dashboard(latest_temperature, latest_humidity, touch_redraw);
        last_lcd_update_ms = now;
        lcd.endWrite();
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
