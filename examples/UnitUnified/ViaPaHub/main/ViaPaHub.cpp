/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using the ENV units via UnitPaHub

  Core ---> PaHub (0x71) -+--> ch:0 UnitCO2    (SCD40 0x62)
                          |
                          +--> ch:1 UnitCO2L   (SCD41 0x62)
                          |
                          +--> ch:2 UnitENVIII (SHT30 0x44 / QMP6988 0x70)
                          |
                          +--> ch:3 UnitENVIV  (SHT40 0x44 / BMP280 0x76)
                          |
                          +--> ch:4 UnitENVPro (BME688 0x77)
                          |
                          +--> ch:5 UnitTVOC   (SGP30 0x58)

  NOTICE:
  - Set the PaHub I2C address to 0x71 with its DIP switch.
    QMP6988 in UnitENVIII uses 0x70, which is the PaHub default address.
    PaHub answers at its own address whichever channel is selected, so the default address collides with QMP6988.
  - UnitCO2 and UnitCO2L share 0x62, but they coexist on separate channels.
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <M5UnitUnifiedHUB.h>                 // UnitPaHub
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware I2C wiring helpers

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

constexpr uint8_t PAHUB_ADDRESS{0x71};  // Must not be 0x70 (QMP6988 in UnitENVIII)
m5::unit::UnitPaHub hub{PAHUB_ADDRESS};

m5::unit::UnitCO2 co2;      // ch:0
m5::unit::UnitCO2L co2l;    // ch:1
m5::unit::UnitENV3 env3;    // ch:2
m5::unit::UnitENV4 env4;    // ch:3
m5::unit::UnitENVPro envp;  // ch:4
m5::unit::UnitTVOC tvoc;    // ch:5

enum Line : uint8_t {
    LineCO2,
    LineCO2L,
    LineSHT30,
    LineQMP6988,
    LineSHT40,
    LineBMP280,
    LineBME688,
    LineSGP30,
    LineMax
};

void draw_line(const Line line, const char* text)
{
    const int32_t y = lcd.fontHeight() * line;
    lcd.startWrite();
    lcd.fillRect(0, y, lcd.width(), lcd.fontHeight(), TFT_BLACK);
    lcd.drawString(text, 0, y);
    lcd.endWrite();
}
}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    if (!hub.add(co2, 0) ||   // PaHub ch:0 -> UnitCO2
        !hub.add(co2l, 1) ||  // PaHub ch:1 -> UnitCO2L
        !hub.add(env3, 2) ||  // PaHub ch:2 -> UnitENVIII
        !hub.add(env4, 3) ||  // PaHub ch:3 -> UnitENVIV
        !hub.add(envp, 4) ||  // PaHub ch:4 -> UnitENVPro
        !hub.add(tvoc, 5)) {  // PaHub ch:5 -> UnitTVOC
        M5_LOGE("Failed to connect units");
        m5::unit::wiring::failStop();
    }

    // Board-aware I2C for the PaHub: NessoN1 -> SoftwareI2C(port_b), NanoC6/NanoH2 -> Ex_I2C, others -> Wire
    bool unit_ready = m5::unit::wiring::addI2C(Units, hub, 400 * 1000U) && Units.begin();
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.fillScreen(TFT_BLACK);
    for (uint8_t i = 0; i < LineMax; ++i) {
        draw_line(static_cast<Line>(i), "-");
    }
    M5.Log.printf("SGP30 measurement starts 15 seconds after begin\n");
}

void loop()
{
    M5.update();
    Units.update();

    char buf[64]{};

    if (co2.updated()) {
        M5.Log.printf(">CO2_CO2:%u\n>CO2_Temp:%.2f\n>CO2_Humidity:%.2f\n", co2.co2(), co2.temperature(),
                      co2.humidity());
        snprintf(buf, sizeof(buf), "CO2   %4u ppm %5.2fC %5.2f%%", co2.co2(), co2.temperature(), co2.humidity());
        draw_line(LineCO2, buf);
    }
    if (co2l.updated()) {
        M5.Log.printf(">CO2L_CO2:%u\n>CO2L_Temp:%.2f\n>CO2L_Humidity:%.2f\n", co2l.co2(), co2l.temperature(),
                      co2l.humidity());
        snprintf(buf, sizeof(buf), "CO2L  %4u ppm %5.2fC %5.2f%%", co2l.co2(), co2l.temperature(), co2l.humidity());
        draw_line(LineCO2L, buf);
    }
    if (env3.sht30.updated()) {
        M5.Log.printf(">SHT30_Temp:%.2f\n>SHT30_Humidity:%.2f\n", env3.sht30.temperature(), env3.sht30.humidity());
        snprintf(buf, sizeof(buf), "SHT30 %5.2fC %5.2f%%", env3.sht30.temperature(), env3.sht30.humidity());
        draw_line(LineSHT30, buf);
    }
    if (env3.qmp6988.updated()) {
        M5.Log.printf(">QMP6988_Temp:%.2f\n>QMP6988_Pressure:%.2f\n", env3.qmp6988.temperature(),
                      env3.qmp6988.pressure() * 0.01f /* To hPa */);
        snprintf(buf, sizeof(buf), "QMP   %5.2fC %7.2fhPa", env3.qmp6988.temperature(),
                 env3.qmp6988.pressure() * 0.01f);
        draw_line(LineQMP6988, buf);
    }
    if (env4.sht40.updated()) {
        M5.Log.printf(">SHT40_Temp:%.2f\n>SHT40_Humidity:%.2f\n", env4.sht40.temperature(), env4.sht40.humidity());
        snprintf(buf, sizeof(buf), "SHT40 %5.2fC %5.2f%%", env4.sht40.temperature(), env4.sht40.humidity());
        draw_line(LineSHT40, buf);
    }
    if (env4.bmp280.updated()) {
        M5.Log.printf(">BMP280_Temp:%.2f\n>BMP280_Pressure:%.2f\n", env4.bmp280.temperature(),
                      env4.bmp280.pressure() * 0.01f /* To hPa */);
        snprintf(buf, sizeof(buf), "BMP   %5.2fC %7.2fhPa", env4.bmp280.temperature(), env4.bmp280.pressure() * 0.01f);
        draw_line(LineBMP280, buf);
    }
    if (envp.updated()) {
        M5.Log.printf(">BME688_Temp:%.2f\n>BME688_Pressure:%.2f\n>BME688_Humidity:%.2f\n>BME688_Gas:%.2f\n",
                      envp.temperature(), envp.pressure() * 0.01f /* To hPa */, envp.humidity(), envp.gas());
        snprintf(buf, sizeof(buf), "BME   %5.2fC %5.2f%% %.0fOhm", envp.temperature(), envp.humidity(), envp.gas());
        draw_line(LineBME688, buf);
    }
    // SGP30 measurement starts 15 seconds after begin
    if (tvoc.updated()) {
        M5.Log.printf(">SGP30_CO2eq:%u\n>SGP30_TVOC:%u\n", tvoc.co2eq(), tvoc.tvoc());
        snprintf(buf, sizeof(buf), "SGP30 %4u ppm %4u ppb", tvoc.co2eq(), tvoc.tvoc());
        draw_line(LineSGP30, buf);
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS   = 2000;
    constexpr TickType_t FEED_SLEEP_TICKS = pdMS_TO_TICKS(5);
    static uint32_t s_next_feed_ms        = 0;
    const uint32_t now_ms                 = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    if (now_ms >= s_next_feed_ms) {
        s_next_feed_ms = now_ms + FEED_INTERVAL_MS;
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
