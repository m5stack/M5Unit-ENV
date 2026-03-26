/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitSHT20
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_SHT20.hpp>
#include <m5_unit_component/adapter_i2c.hpp>
#include <chrono>
#include <cmath>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::sht20;
using namespace m5::unit::sht20::command;

constexpr size_t STORED_SIZE{4};

#if defined(USING_HAT_SHT20)
namespace hat {
struct I2cPins {
    int sda, scl;
};

I2cPins get_hat_pins(const m5::board_t board)
{
    switch (board) {
        case m5::board_t::board_M5StickC:
        case m5::board_t::board_M5StickCPlus:
        case m5::board_t::board_M5StickCPlus2:
            return {0, 26};
        case m5::board_t::board_M5StickS3:
            return {8, 0};
        case m5::board_t::board_M5StackCoreInk:
            return {25, 26};
        case m5::board_t::board_ArduinoNessoN1:
            return {6, 7};
        default:
            return {-1, -1};
    }
}
}  // namespace hat
#endif

class TestSHT20 : public I2CComponentTestBase<UnitSHT20> {
protected:
    virtual UnitSHT20* get_instance() override
    {
        auto ptr         = new m5::unit::UnitSHT20();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
#if defined(USING_HAT_SHT20)
    virtual bool begin() override
    {
        auto board      = M5.getBoard();
        const auto pins = hat::get_hat_pins(board);
        auto& wire      = (board == m5::board_t::board_ArduinoNessoN1) ? Wire1 : Wire;
        wire.end();
        wire.begin(pins.sda, pins.scl, unit->component_config().clock);
        return Units.add(*unit, wire) && Units.begin();
    }
#endif
};

namespace {

// SHT20 CRC-8 reference implementation for test verification
// polynomial x^8 + x^5 + x^4 + 1 = 0x131, init = 0x00
uint8_t crc8_sht20(const uint8_t* data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint_fast8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
    }
    return crc;
}

void check_measurement_values(UnitSHT20* u)
{
    EXPECT_TRUE(std::isfinite(u->latest().celsius()));
    EXPECT_TRUE(std::isfinite(u->latest().fahrenheit()));
    EXPECT_TRUE(std::isfinite(u->latest().humidity()));
}

}  // namespace

TEST_F(TestSHT20, SingleShot)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    uint32_t cnt{10};  // repeat 10 times
    while (cnt--) {
        sht20::Data d{};
        EXPECT_TRUE(unit->measureSingleshot(d));
        EXPECT_TRUE(std::isfinite(d.temperature()));
        EXPECT_TRUE(std::isfinite(d.humidity()));

        // Sanity range check: temperature -40..125, humidity 0..100
        EXPECT_GE(d.celsius(), -40.f);
        EXPECT_LE(d.celsius(), 125.f);
        EXPECT_GE(d.humidity(), 0.f);
        EXPECT_LE(d.humidity(), 100.f);
    }
}

TEST_F(TestSHT20, Periodic)
{
    SCOPED_TRACE(ustr);

    auto ad     = unit->asAdapter<m5::unit::AdapterI2C>(m5::unit::Adapter::Type::I2C);
    bool is_bus = ad && ad->implType() == m5::unit::AdapterI2C::ImplType::Bus;

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_TRUE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->inPeriodic());

    M5_LOGI("Periodic: interval:%lu ms", (unsigned long)unit->interval());

    // Cannot call singleshot in periodic
    {
        sht20::Data d{};
        EXPECT_FALSE(unit->measureSingleshot(d));
    }

    {
        uint32_t timeout = is_bus ? std::max<uint32_t>(unit->interval(), 500) * (STORED_SIZE + 1) * 4
                                  : unit->interval() * (STORED_SIZE + 1);
        auto r           = collect_periodic_measurements(unit.get(), STORED_SIZE, timeout, check_measurement_values);
        EXPECT_FALSE(r.timed_out);
        EXPECT_EQ(r.update_count, STORED_SIZE);
        EXPECT_LE(r.median(), r.expected_interval + (is_bus ? 5U : 1U));
    }

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_EQ(unit->available(), STORED_SIZE);
    EXPECT_FALSE(unit->empty());
    EXPECT_TRUE(unit->full());

    uint32_t cnt{2};
    while (cnt-- && unit->available()) {
        EXPECT_TRUE(std::isfinite(unit->temperature()));
        EXPECT_TRUE(std::isfinite(unit->humidity()));
        EXPECT_FLOAT_EQ(unit->temperature(), unit->oldest().temperature());
        EXPECT_FLOAT_EQ(unit->humidity(), unit->oldest().humidity());
        EXPECT_FALSE(unit->empty());
        unit->discard();
    }
    EXPECT_EQ(unit->available(), STORED_SIZE - 2);
    EXPECT_FALSE(unit->empty());
    EXPECT_FALSE(unit->full());

    unit->flush();
    EXPECT_EQ(unit->available(), 0);
    EXPECT_TRUE(unit->empty());
    EXPECT_FALSE(unit->full());

    EXPECT_FALSE(std::isfinite(unit->temperature()));
    EXPECT_FALSE(std::isfinite(unit->humidity()));
}

TEST_F(TestSHT20, Resolution)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    constexpr std::tuple<const char*, Resolution> table[] = {
        {"RH12_T14", Resolution::RH12_T14},
        {"RH8_T12", Resolution::RH8_T12},
        {"RH10_T13", Resolution::RH10_T13},
        {"RH11_T11", Resolution::RH11_T11},
    };

    for (auto&& e : table) {
        const char* s{};
        Resolution res{};
        std::tie(s, res) = e;
        SCOPED_TRACE(s);

        EXPECT_TRUE(unit->writeResolution(res));

        Resolution readback{};
        EXPECT_TRUE(unit->readResolution(readback));
        EXPECT_EQ(readback, res);

        // Verify measurement works at each resolution
        sht20::Data d{};
        EXPECT_TRUE(unit->measureSingleshot(d));
        EXPECT_TRUE(std::isfinite(d.temperature()));
        EXPECT_TRUE(std::isfinite(d.humidity()));
    }

    // Restore default
    EXPECT_TRUE(unit->writeResolution(Resolution::RH12_T14));
}

TEST_F(TestSHT20, Heater)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    EXPECT_TRUE(unit->startHeater());

    // Read user register to verify heater is on
    uint8_t reg{};
    uint8_t cmd = READ_USER_REG;
    EXPECT_EQ(unit->writeWithTransaction(&cmd, 1), m5::hal::error::error_t::OK);
    EXPECT_EQ(unit->readWithTransaction(&reg, 1), m5::hal::error::error_t::OK);
    EXPECT_TRUE(reg & REG_HEATER_BIT);

    EXPECT_TRUE(unit->stopHeater());

    EXPECT_EQ(unit->writeWithTransaction(&cmd, 1), m5::hal::error::error_t::OK);
    EXPECT_EQ(unit->readWithTransaction(&reg, 1), m5::hal::error::error_t::OK);
    EXPECT_FALSE(reg & REG_HEATER_BIT);
}

TEST_F(TestSHT20, SoftReset)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    // Change resolution and enable heater, then reset
    EXPECT_TRUE(unit->writeResolution(Resolution::RH8_T12));
    EXPECT_TRUE(unit->startHeater());
    EXPECT_TRUE(unit->softReset());

    // After reset, resolution returns to default (RH12_T14)
    Resolution res{};
    EXPECT_TRUE(unit->readResolution(res));
    EXPECT_EQ(res, Resolution::RH12_T14);

    // Datasheet: "Reinitializes to default settings (except heater bit)"
    // Heater bit is NOT cleared by soft reset
    uint8_t reg{};
    uint8_t cmd = READ_USER_REG;
    EXPECT_EQ(unit->writeWithTransaction(&cmd, 1), m5::hal::error::error_t::OK);
    EXPECT_EQ(unit->readWithTransaction(&reg, 1), m5::hal::error::error_t::OK);
    EXPECT_TRUE(reg & REG_HEATER_BIT);

    // Clean up: disable heater
    EXPECT_TRUE(unit->stopHeater());
}
