/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for Unit/HatENVIII
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware I2C wiring helpers

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_ENV3) && !defined(USING_HAT_ENV3)
// For Unit ENVIII (U001-C)
// #define USING_UNIT_ENV3
// For Hat ENVIII (U053-D)
// #define USING_HAT_ENV3
#endif
// *************************************************************

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

#if defined(USING_UNIT_ENV3)
#pragma message "Using UnitENV3"
m5::unit::UnitENV3 unit;
#elif defined(USING_HAT_ENV3)
#pragma message "Using HatENV3"
m5::unit::HatENV3 unit;
#else
#error "Choose unit or hat"
#endif

auto& sht30   = unit.sht30;
auto& qmp6988 = unit.qmp6988;

}  // namespace

void setup()
{
    auto m5cfg = M5.config();
#if defined(USING_HAT_ENV3)
    m5cfg.pmic_button  = false;  // Disable BtnPWR
    m5cfg.internal_imu = false;  // Disable internal IMU
    m5cfg.internal_rtc = false;  // Disable internal RTC
#endif
    M5.begin(m5cfg);

    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

#if defined(USING_HAT_ENV3)
    // Hat: board-aware Hat I2C (StickC series Wire / NessoN1 Wire1)
    const bool unit_ready = m5::unit::wiring::addHatI2C(Units, unit) && Units.begin();
#else
    // Board-aware I2C: NessoN1 -> SoftwareI2C(port_b), NanoC6/NanoH2 -> Ex_I2C, others -> Wire
    const bool unit_ready = m5::unit::wiring::addI2C(Units, unit) && Units.begin();
#endif
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
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

    if (sht30.updated()) {
        M5.Log.printf(">SHT30Temp:%2.2f\n>Humidity:%2.2f\n", sht30.temperature(), sht30.humidity());
        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 3, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("SHT30\nTemp:%2.2f\nHumidity:%2.2f", sht30.temperature(), sht30.humidity());
        lcd.endWrite();
    }
    if (qmp6988.updated()) {
        M5.Log.printf(">QMP6988Temp:%2.2f\n>Pressure:%.2f\n>Altitude:%.2f\n", qmp6988.temperature(),
                      qmp6988.pressure() * 0.01f, qmp6988.altitude());
        lcd.startWrite();
        lcd.fillRect(0, lcd.fontHeight() * 3, lcd.width(), lcd.fontHeight() * 4, TFT_BLACK);
        lcd.setCursor(0, lcd.fontHeight() * 3);
        lcd.printf("QMP6988\nTemp:%2.2f\nPressure:%.2f\nAltitude:%.2f", qmp6988.temperature(),
                   qmp6988.pressure() * 0.01f, qmp6988.altitude());
        lcd.endWrite();
    }

    if (M5.BtnA.wasClicked()) {
        sht30.stopPeriodicMeasurement();
        qmp6988.stopPeriodicMeasurement();

        m5::unit::sht30::Data ds{};
        if (sht30.measureSingleshot(ds)) {
            M5.Log.printf("== Singleshot SHT30 Temp:%2.2f Humidity:%2.2f\n", ds.temperature(), ds.humidity());
        } else {
            M5_LOGW("Single SHT30 failed");
        }
        m5::unit::qmp6988::Data dq{};
        if (qmp6988.measureSingleshot(dq)) {
            M5.Log.printf("== Singleshot QMP6988 Temp:%2.2f Pressure:%.2f\n", dq.temperature(), dq.pressure() * 0.01f);
        } else {
            M5_LOGW("Single QMP failed");
        }

        sht30.startPeriodicMeasurement();
        qmp6988.startPeriodicMeasurement();
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
