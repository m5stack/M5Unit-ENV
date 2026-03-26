/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for HatYun (STM32 LED/Light controller at 0x38)
  Requires physical Hat Yun connected to StickC / StickCPlus / StickCPlus2
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_HatYun.hpp>
#include <m5_unit_component/adapter_i2c.hpp>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::hatyun;
using namespace m5::unit::hatyun::command;

constexpr size_t STORED_SIZE{4};

class TestHatYun : public I2CComponentTestBase<HatYun> {
protected:
    virtual HatYun* get_instance() override
    {
        auto ptr         = new m5::unit::HatYun();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
    virtual bool begin() override
    {
        auto board = M5.getBoard();
        int sda = -1, scl = -1;
        switch (board) {
            case m5::board_t::board_M5StickC:
            case m5::board_t::board_M5StickCPlus:
            case m5::board_t::board_M5StickCPlus2:
                sda = 0;
                scl = 26;
                break;
            default:
                break;
        }
        if (sda < 0 || scl < 0) {
            return false;
        }
        Wire.end();
        Wire.begin(sda, scl, 400 * 1000U);
        return Units.add(*unit, Wire) && Units.begin();
    }
};

TEST_F(TestHatYun, ReadLight)
{
    SCOPED_TRACE(ustr);

    uint16_t value{};
    EXPECT_TRUE(unit->readLight(value));
    M5_LOGI("Light: %u", value);
}

TEST_F(TestHatYun, WriteLED)
{
    SCOPED_TRACE(ustr);

    // Set each LED individually
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        EXPECT_TRUE(unit->writeLED(i, 32, 0, 0));  // dim red
    }
    m5::utility::delay(200);

    // All green
    EXPECT_TRUE(unit->writeAllLED(0, 32, 0));
    m5::utility::delay(200);

    // All blue
    EXPECT_TRUE(unit->writeAllLED(0, 0, 32));
    m5::utility::delay(200);

    // Turn off
    EXPECT_TRUE(unit->writeAllLED(0, 0, 0));
}

TEST_F(TestHatYun, WriteRainbow)
{
    SCOPED_TRACE(ustr);

    for (uint8_t offset = 0; offset < 64; offset += 8) {
        EXPECT_TRUE(unit->writeRainbow(offset, 32));
        m5::utility::delay(50);
    }

    EXPECT_TRUE(unit->writeAllLED(0, 0, 0));
}

TEST_F(TestHatYun, Periodic)
{
    SCOPED_TRACE(ustr);

    auto ad     = unit->asAdapter<m5::unit::AdapterI2C>(m5::unit::Adapter::Type::I2C);
    bool is_bus = ad && ad->implType() == m5::unit::AdapterI2C::ImplType::Bus;

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    // Start with 100ms interval for testing
    EXPECT_TRUE(unit->startPeriodicMeasurement(100));
    EXPECT_TRUE(unit->inPeriodic());

    M5_LOGI("Periodic: interval:%lu ms", (unsigned long)unit->interval());

    {
        uint32_t timeout = is_bus ? std::max<uint32_t>(unit->interval(), 500) * (STORED_SIZE + 1) * 4
                                  : unit->interval() * (STORED_SIZE + 1);
        auto r           = collect_periodic_measurements(unit.get(), STORED_SIZE, timeout);
        EXPECT_FALSE(r.timed_out);
        EXPECT_EQ(r.update_count, STORED_SIZE);
        EXPECT_LE(r.median(), r.expected_interval + (is_bus ? 5U : 1U));
    }

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_EQ(unit->available(), STORED_SIZE);
    EXPECT_FALSE(unit->empty());
    EXPECT_TRUE(unit->full());

    // Verify light values are readable
    uint32_t cnt{2};
    while (cnt-- && unit->available()) {
        M5_LOGI("Light: %u", unit->oldest().light);
        EXPECT_FALSE(unit->empty());
        unit->discard();
    }
    EXPECT_EQ(unit->available(), STORED_SIZE - 2);

    unit->flush();
    EXPECT_EQ(unit->available(), 0);
    EXPECT_TRUE(unit->empty());

    // Verify restart works
    EXPECT_TRUE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
}
