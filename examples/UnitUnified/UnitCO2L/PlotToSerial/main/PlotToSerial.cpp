/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitCO2L
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware I2C wiring helpers

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitCO2L unit;

}  // namespace

using namespace m5::unit::scd4x;

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

    {
        auto ret = unit.stopPeriodicMeasurement();
        float offset{};
        ret &= unit.readTemperatureOffset(offset);
        uint16_t altitude{};
        ret &= unit.readSensorAltitude(altitude);
        uint16_t pressure{};
        ret &= unit.readAmbientPressure(pressure);
        bool asc{};
        ret &= unit.readAutomaticSelfCalibrationEnabled(asc);
        uint16_t ppm{};
        ret &= unit.readAutomaticSelfCalibrationTarget(ppm);
        uint16_t initialPeriod{}, standardPeriod{};
        ret &= unit.readAutomaticSelfCalibrationInitialPeriod(initialPeriod);
        ret &= unit.readAutomaticSelfCalibrationStandardPeriod(standardPeriod);

        ret &= unit.startPeriodicMeasurement();

        M5.Log.printf(
            "     temp offset:%f\n"
            " sensor altitude:%u\n"
            "ambient pressure:%u\n"
            "     ASC enabled:%u\n"
            "      ASC target:%u\n"
            "  initial period:%u\n"
            " standard period:%u\n",
            offset, altitude, pressure, asc, ppm, initialPeriod, standardPeriod);

        if (!ret) {
            m5::unit::wiring::failStop();
        }
    }
    lcd.fillScreen(TFT_DARKGREEN);
}

void loop()
{
    M5.update();

    // Periodic
    Units.update();
    if (unit.updated()) {
        // Can be checked e.g. by serial plotters
        M5.Log.printf(">CO2:%u\n>Temperature:%2.2f\n>Humidity:%2.2f\n", unit.co2(), unit.temperature(),
                      unit.humidity());
        lcd.startWrite();
        lcd.fillRect(0, 0, lcd.width(), lcd.fontHeight() * 3, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("CO2:%u\nTemperature:%2.2f\nHumidity:%2.2f", unit.co2(), unit.temperature(), unit.humidity());
        lcd.endWrite();
    }

    // Single shot
    if (M5.BtnA.wasClicked()) {
        static bool all{};  // false: RHT only
        all = !all;
        M5.Log.printf("Try single shot %u, waiting measurement...\n", all);
        unit.stopPeriodicMeasurement();
        Data d{};
        if (all) {
            if (unit.measureSingleshot(d)) {
                M5.Log.printf("  SingleAll: CO2:%u T:%2.2f H:%2.2f\n", d.co2(), d.temperature(), d.humidity());
            }
        } else {
            if (unit.measureSingleshotRHT(d)) {
                M5.Log.printf("  SingleRHT: T:%2.2f H:%2.2f\n", d.temperature(), d.humidity());
            }
        }
        unit.startPeriodicMeasurement();
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
