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
#include <M5HAL.hpp>  // For NessoN1
#include <cmath>

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitENV4 unit;
auto& sht40  = unit.sht40;
auto& bmp280 = unit.bmp280;

float calculate_altitude(const float pressure, const float seaLevelHPa = 1013.25f)
{
    return 44330.f * (1.0f - pow((pressure / 100.f) / seaLevelHPa, 0.1903f));
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

    {
        using namespace m5::unit::bmp280;
        auto cfg             = bmp280.config();
        cfg.osrs_pressure    = Oversampling::X16;
        cfg.osrs_temperature = Oversampling::X2;
        cfg.filter           = Filter::Coeff16;
        cfg.standby          = Standby::Time500ms;
        bmp280.config(cfg);
    }

    auto board = M5.getBoard();

    // NessoN1: Arduino Wire (I2C_NUM_0) cannot be used for GROVE port.
    //   Wire is used by M5Unified In_I2C for internal devices (IOExpander etc.).
    //   Solution: Use SoftwareI2C via M5HAL (bit-banging) for the GROVE port.
    // NanoC6: Wire.begin() on GROVE pins conflicts with m5::I2C_Class registered by Ex_I2C.setPort()
    //   on the same I2C_NUM_0, causing sporadic NACK errors.
    //   Solution: Use M5.Ex_I2C (m5::I2C_Class) directly instead of Arduino Wire.
    bool unit_ready{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a (which maps to Wire pins 8/10)
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        unit_ready = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
        unit_ready = Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
        unit_ready = Units.add(unit, Wire) && Units.begin();
    }
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
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
        auto p = bmp280.pressure();
        M5.Log.printf(
            ">BMP280Temp:%.4f\n"
            ">Pressure:%.4f\n"
            ">Altitude:%.4f\n",
            bmp280.temperature(), p * 0.01f /* To hPa */, calculate_altitude(p));

        lcd.startWrite();
        lcd.fillRect(0, lcd.fontHeight() * 3, lcd.width(), lcd.fontHeight() * 4, TFT_BLACK);
        lcd.setCursor(0, lcd.fontHeight() * 3);
        lcd.printf(
            "BMP280\n"
            "Temp:%.4f\n"
            "Pressure:%.4f\n"
            "Altitude:%.4f",
            bmp280.temperature(), p * 0.01f /* To hPa */, calculate_altitude(p));
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
