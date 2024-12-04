/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitSHT30
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_SHT40.hpp>
#include <chrono>
#include <cmath>
#include <random>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::sht40;
using namespace m5::unit::sht40::command;
using m5::unit::types::elapsed_time_t;

constexpr size_t STORED_SIZE{4};

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestSHT40 : public ComponentTestBase<UnitSHT40, bool> {
protected:
    virtual UnitSHT40* get_instance() override
    {
        auto ptr         = new m5::unit::UnitSHT40();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSHT40,
//                         ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSHT40, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSHT40, ::testing::Values(false));

namespace {

template <class U>
elapsed_time_t test_periodic(U* unit, const uint32_t times)
{
    auto timeout_at = m5::utility::millis() + (times * unit->interval() * 2);

    // First read
    while (!unit->updated() && m5::utility::millis() < timeout_at) {
        std::this_thread::yield();
        unit->update();
    }
    // timeout
    if (!unit->updated()) {
        return 0;
    }

    //
    uint32_t measured{};
    auto start_at = m5::utility::millis();
    unit->update();

    do {
        m5::utility::delay(1);
        unit->update();
        measured += unit->updated() ? 1 : 0;
    } while (measured < times && m5::utility::millis() < timeout_at);

    return (measured == times) ? m5::utility::millis() - start_at : 0;
}

std::tuple<const char*, Precision, Heater, elapsed_time_t> sm_table[] = {
    //
    {"HighLong", Precision::High, Heater::Long, 9},
    {"HighShort", Precision::High, Heater::Short, 9},
    {"HighNone", Precision::High, Heater::None, 9},
    //
    {"MediumLong", Precision::Medium, Heater::Long, 5},
    {"MediumSHort", Precision::Medium, Heater::Short, 5},
    {"MediumNone", Precision::Medium, Heater::None, 5},
    //
    {"LowLong", Precision::Low, Heater::Long, 2},
    {"LowShort", Precision::Low, Heater::Short, 2},
    {"LowNone", Precision::Low, Heater::None, 2},
};

}  // namespace

TEST_P(TestSHT40, SoftReset)
{
    SCOPED_TRACE(ustr);
    // Soft reset is only possible in standby mode.
    EXPECT_FALSE(unit->softReset());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->softReset());
}

TEST_P(TestSHT40, GeneralReset)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->generalReset());
}

TEST_P(TestSHT40, SerialNumber)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    {
        // Read direct [MSB] SNB_3, SNB_2, CRC, SNB_1, SNB_0, CRC [LSB]
        std::array<uint8_t, 6> rbuf{};
        EXPECT_TRUE(unit->readRegister(GET_SERIAL_NUMBER, rbuf.data(), rbuf.size(), 1));
        uint32_t d_sno = (((uint32_t)rbuf[0]) << 24) | (((uint32_t)rbuf[1]) << 16) | (((uint32_t)rbuf[3]) << 8) |
                         ((uint32_t)rbuf[4]);

        //
        uint32_t sno{};
        char ssno[9]{};
        EXPECT_TRUE(unit->readSerialNumber(sno));
        EXPECT_TRUE(unit->readSerialNumber(ssno));

        EXPECT_EQ(sno, d_sno);

        // M5_LOGI("s:[%s] uint32:[%x]", ssno, sno);

        std::stringstream stream;
        stream << std::uppercase << std::setw(8) << std::setfill('0') << std::hex << sno;
        std::string s(stream.str());
        EXPECT_STREQ(s.c_str(), ssno);
    }
}

TEST_P(TestSHT40, SingleShot)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& e : sm_table) {
        const char* s{};
        Precision p{};
        Heater h{};
        elapsed_time_t tm{};
        std::tie(s, p, h, tm) = e;

        SCOPED_TRACE(s);

        uint32_t cnt{5};  // repeat 5 times
        while (cnt--) {
            sht40::Data d{};
            EXPECT_TRUE(unit->measureSingleshot(d, p, h));
            EXPECT_TRUE(std::isfinite(d.temperature()));
            EXPECT_TRUE(std::isfinite(d.humidity()));
            EXPECT_EQ(d.heater, h != Heater::None);
        }
    }
}

TEST_P(TestSHT40, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& e : sm_table) {
        const char* s{};
        Precision p{};
        Heater h{};
        elapsed_time_t tm{};
        std::tie(s, p, h, tm) = e;

        SCOPED_TRACE(s);

        EXPECT_TRUE(unit->startPeriodicMeasurement(p, h));
        EXPECT_TRUE(unit->inPeriodic());

        // Cannot call all singleshot in periodic
        for (auto&& single : sm_table) {
            const char* s{};
            Precision p{};
            Heater h{};
            elapsed_time_t tm{};
            std::tie(s, p, h, tm) = single;
            sht40::Data d{};
            EXPECT_FALSE(unit->measureSingleshot(d, p, h));
        }
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        //
        EXPECT_TRUE(unit->startPeriodicMeasurement(p, h));
        EXPECT_TRUE(unit->inPeriodic());

        auto elapsed = test_periodic(unit.get(), STORED_SIZE);
        EXPECT_NE(elapsed, 0);
        EXPECT_GE(elapsed, STORED_SIZE * tm);
        EXPECT_LE(elapsed, STORED_SIZE * tm + 1);

        // M5_LOGW("[%s] %lu %zu", s, elapsed, unit->available());

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_EQ(unit->available(), STORED_SIZE);
        EXPECT_FALSE(unit->empty());
        EXPECT_TRUE(unit->full());

        uint32_t cnt{2};
        while (cnt-- && unit->available()) {
            EXPECT_TRUE(std::isfinite(unit->temperature()));
            EXPECT_TRUE(std::isfinite(unit->fahrenheit()));
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
}
