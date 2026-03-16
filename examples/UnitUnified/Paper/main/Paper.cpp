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
#include <cstdio>

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitSHT30 sht30;
float latest_temperature{};
float latest_humidity{};
bool has_measurement{};
uint32_t last_lcd_update_ms{};

void draw_dashboard(const float temperature, const float humidity)
{
    char temp_text[32];
    char hum_text[32];
    std::snprintf(temp_text, sizeof(temp_text), "%7.2f C ", temperature);
    std::snprintf(hum_text, sizeof(hum_text), "%7.2f %% ", humidity);

    const int w = lcd.width();
    const int h = lcd.height();

    constexpr int value_x      = 220;
    constexpr int temp_value_y = 130;
    const int humid_value_y    = h / 2 + 58;

    static bool initialized{};
    lcd.startWrite();
    if (!initialized) {
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

    lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    lcd.setTextSize(3.5f);
    lcd.setCursor(value_x, temp_value_y);
    lcd.print(temp_text);
    lcd.setCursor(value_x, humid_value_y);
    lcd.print(hum_text);
    lcd.endWrite();
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
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
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
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
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
        touch_redraw  = td.wasPressed() || td.wasClicked();
    }

    if (sht30.updated()) {
        latest_temperature = sht30.temperature();
        latest_humidity    = sht30.humidity();
        has_measurement    = true;
        M5.Log.printf(">SHT30Temp:%2.2f\n>Humidity:%2.2f\n", latest_temperature, latest_humidity);
    }

    if (has_measurement && (now - last_lcd_update_ms >= 60 * 1000U || touch_redraw)) {
        draw_dashboard(latest_temperature, latest_humidity);
        last_lcd_update_ms = now;
    }
}
