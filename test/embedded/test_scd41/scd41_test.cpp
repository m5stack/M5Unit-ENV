/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitSCD41
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <unit/unit_SCD41.hpp>
#include <chrono>
#include <iostream>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::scd4x;
using m5::unit::types::elapsed_time_t;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

constexpr uint32_t STORED_SIZE{4};

class TestSCD4x : public ComponentTestBase<UnitSCD41, bool> {
protected:
    virtual UnitSCD41* get_instance() override
    {
        auto ptr         = new m5::unit::UnitSCD41();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        auto cfg           = ptr->config();
        cfg.start_periodic = false;
        ptr->config(cfg);
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD4x, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD4x, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD4x, ::testing::Values(false));

namespace {
}  // namespace

#include "../scd4x_test.inl"

TEST_P(TestSCD4x, Singleshot)
{
    SCOPED_TRACE(ustr);
    {
        Data d{};
        EXPECT_FALSE(unit->inPeriodic());
        EXPECT_TRUE(unit->measureSingleshot(d));
        EXPECT_NE(d.co2(), 0);
        EXPECT_TRUE(std::isfinite(d.temperature()));
        EXPECT_TRUE(std::isfinite(d.humidity()));

        EXPECT_TRUE(unit->startPeriodicMeasurement());

        EXPECT_TRUE(unit->inPeriodic());
        EXPECT_FALSE(unit->measureSingleshot(d));
        EXPECT_EQ(d.co2(), 0);
        EXPECT_FLOAT_EQ(d.temperature(), -45.f);
        EXPECT_FLOAT_EQ(d.humidity(), 0.0f);

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
    }
    {
        Data d{};
        EXPECT_FALSE(unit->inPeriodic());
        EXPECT_TRUE(unit->measureSingleshotRHT(d));
        EXPECT_EQ(d.co2(), 0);
        EXPECT_TRUE(std::isfinite(d.temperature()));
        EXPECT_TRUE(std::isfinite(d.humidity()));

        EXPECT_TRUE(unit->startPeriodicMeasurement());

        EXPECT_TRUE(unit->inPeriodic());
        EXPECT_FALSE(unit->measureSingleshotRHT(d));
        EXPECT_EQ(d.co2(), 0);
        EXPECT_FLOAT_EQ(d.temperature(), -45.f);
        EXPECT_FLOAT_EQ(d.humidity(), 0.0f);

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
    }
}

TEST_P(TestSCD4x, PowerMode)
{
    SCOPED_TRACE(ustr);

    EXPECT_FALSE(unit->inPeriodic());

    uint32_t count{8};
    while (count--) {
        EXPECT_TRUE(unit->powerDown()) << count;
        EXPECT_TRUE(unit->wakeup()) << count;
    }

    EXPECT_TRUE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->inPeriodic());

    EXPECT_FALSE(unit->powerDown());
    EXPECT_FALSE(unit->wakeup());

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_TRUE(unit->reInit());
}

TEST_P(TestSCD4x, ASC)
{
    SCOPED_TRACE(ustr);

    constexpr uint16_t hours_table[] = {0, 32768, 65535};
    for (auto&& h : hours_table) {
        EXPECT_TRUE(unit->writeAutomaticSelfCalibrationInitialPeriod(h));
        EXPECT_TRUE(unit->writeAutomaticSelfCalibrationStandardPeriod(h));

        uint16_t ih{}, sh{};

        EXPECT_TRUE(unit->readAutomaticSelfCalibrationInitialPeriod(ih));
        EXPECT_TRUE(unit->readAutomaticSelfCalibrationStandardPeriod(sh));

        EXPECT_EQ(ih, (h >> 2) << 2);
        EXPECT_EQ(sh, (h >> 2) << 2);
    }

    EXPECT_TRUE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->inPeriodic());
    for (auto&& h : hours_table) {
        EXPECT_FALSE(unit->writeAutomaticSelfCalibrationInitialPeriod(h));
        EXPECT_FALSE(unit->writeAutomaticSelfCalibrationStandardPeriod(h));
    }
}
