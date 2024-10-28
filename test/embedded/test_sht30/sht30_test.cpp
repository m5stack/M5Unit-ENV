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
#include <unit/unit_SHT30.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <bitset>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::sht30;
using namespace m5::unit::sht30::command;

constexpr size_t STORED_SIZE{4};

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestSHT30 : public ComponentTestBase<UnitSHT30, bool> {
protected:
    virtual UnitSHT30* get_instance() override
    {
        auto ptr         = new m5::unit::UnitSHT30();
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

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSHT30,
//                         ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSHT30, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSHT30, ::testing::Values(false));

namespace {
// flot t uu int16 (temperature)
constexpr uint16_t float_to_uint16(const float f)
{
    return f * 65536 / 175;
}

std::tuple<const char*, Repeatability, bool> ss_table[] = {
    {"HighTrue", Repeatability::High, true},       {"MediumTrue", Repeatability::Medium, true},
    {"LowTrue", Repeatability::Low, true},         {"HighFalse", Repeatability::High, false},
    {"MediumFalse", Repeatability::Medium, false}, {"LowFalse", Repeatability::Low, false},
};

void check_measurement_values(UnitSHT30* u)
{
    EXPECT_TRUE(std::isfinite(u->latest().celsius()));
    EXPECT_TRUE(std::isfinite(u->latest().fahrenheit()));
    EXPECT_TRUE(std::isfinite(u->latest().humidity()));
}

}  // namespace

TEST_P(TestSHT30, SingleShot)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    for (auto&& e : ss_table) {
        const char* s{};
        Repeatability rep;
        bool stretch{};
        std::tie(s, rep, stretch) = e;
        SCOPED_TRACE(s);

        int cnt{10};  // repeat 10 times
        while (cnt--) {
            sht30::Data d{};
            EXPECT_TRUE(unit->measureSingleshot(d, rep, stretch)) << (int)rep << " : " << stretch;
            EXPECT_TRUE(std::isfinite(d.temperature()));
            EXPECT_TRUE(std::isfinite(d.humidity()));
        }
    }
}

TEST_P(TestSHT30, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    constexpr std::tuple<const char*, MPS, Repeatability> table[] = {
        //
        {"HalfHigh", MPS::Half, Repeatability::High},
        {"HalfMedium", MPS::Half, Repeatability::Medium},
        {"HalfLow", MPS::Half, Repeatability::Low},
        //
        {"1High", MPS::One, Repeatability::High},
        {"1Medium", MPS::One, Repeatability::Medium},
        {"1Low", MPS::One, Repeatability::Low},
        //
        {"2High", MPS::Two, Repeatability::High},
        {"2Medium", MPS::Two, Repeatability::Medium},
        {"2Low", MPS::Two, Repeatability::Low},
        //
        {"4fHigh", MPS::Four, Repeatability::High},
        {"4Medium", MPS::Four, Repeatability::Medium},
        {"4Low", MPS::Four, Repeatability::Low},
        //
        {"10fHigh", MPS::Ten, Repeatability::High},
        {"10Medium", MPS::Ten, Repeatability::Medium},
        {"10Low", MPS::Ten, Repeatability::Low},
    };

    for (auto&& e : table) {
        const char* s{};
        MPS mps;
        Repeatability rep;
        std::tie(s, mps, rep) = e;
        SCOPED_TRACE(s);

        EXPECT_TRUE(unit->startPeriodicMeasurement(mps, rep));
        EXPECT_TRUE(unit->inPeriodic());

        // Cannot call all singleshot in periodic
        for (auto&& e : ss_table) {
            const char* s{};
            Repeatability rep;
            bool stretch{};
            std::tie(s, rep, stretch) = e;
            sht30::Data d{};

            SCOPED_TRACE(s);
            EXPECT_FALSE(unit->measureSingleshot(d, rep, stretch));
        }
        test_periodic_measurement(unit.get(), 4, 1, check_measurement_values);
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_EQ(unit->available(), STORED_SIZE);
        EXPECT_FALSE(unit->empty());
        EXPECT_TRUE(unit->full());

        uint32_t cnt{2};
        while (cnt-- && unit->available()) {
            // M5_LOGI("%s T:%f H:%f", s, unit->temperature(), unit->humidity());

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

    // ART Command (4 mps)
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_FALSE(unit->writeModeAccelerateResponseTime());
    EXPECT_TRUE(unit->startPeriodicMeasurement(MPS::Half,
                                               Repeatability::High));  // 0.5mps
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_EQ(unit->updatedMillis(), 0);
    EXPECT_TRUE(unit->writeModeAccelerateResponseTime());  // boost to 4mps

    // Cannot call all singleshot in periodic
    for (auto&& e : ss_table) {
        const char* s{};
        Repeatability rep;
        bool stretch{};
        std::tie(s, rep, stretch) = e;
        sht30::Data d{};

        SCOPED_TRACE(s);
        EXPECT_FALSE(unit->measureSingleshot(d, rep, stretch));
    }

    test_periodic_measurement(unit.get(), 4, 1, check_measurement_values);
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_EQ(unit->available(), STORED_SIZE);
    EXPECT_FALSE(unit->empty());
    EXPECT_TRUE(unit->full());
    // M5_LOGI("ART T:%f H:%f", unit->temperature(), unit->humidity());

    EXPECT_TRUE(std::isfinite(unit->temperature()));
    EXPECT_TRUE(std::isfinite(unit->humidity()));
    EXPECT_FLOAT_EQ(unit->temperature(), unit->oldest().temperature());
    EXPECT_FLOAT_EQ(unit->humidity(), unit->oldest().humidity());

    unit->flush();
    EXPECT_TRUE(std::isnan(unit->temperature()));
    EXPECT_TRUE(std::isnan(unit->humidity()));
    EXPECT_EQ(unit->available(), 0);
    EXPECT_TRUE(unit->empty());
    EXPECT_FALSE(unit->full());

    // startPeriodicMeasurement after ART (ART is disabled)
    EXPECT_TRUE(unit->startPeriodicMeasurement(MPS::Two,
                                               Repeatability::High));  // 2 mps
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_EQ(unit->updatedMillis(), 0);

    std::array<uint8_t, 6> rbuf{};
    types::elapsed_time_t timeout_at{}, now{}, at[2]{};
    uint32_t idx{};
    timeout_at = m5::utility::millis() + 1100;
    do {
        m5::utility::delay(1);
        at[idx] = now = m5::utility::millis();
        unit->update();
        if (unit->updated()) {
            ++idx;
        }
    } while (idx < 2 && now <= timeout_at);
    EXPECT_EQ(idx, 2);
    auto diff = at[1] - at[0];
    EXPECT_GT(diff, 250);  // 2mps(500) > 4mps(250)
}

namespace {
void printStatus(const Status& s)
{
#if 0
    std::bitset<16> bits(s.value);
    M5_LOGI("[%s]: %u/%u/%u/%u/%u/%u/%u", bits.to_string().c_str(),
            s.alertPending(), s.heater(), s.trackingAlertRH(),
            s.trackingAlert(), s.reset(), s.command(), s.checksum());
#endif
}
}  // namespace

TEST_P(TestSHT30, HeaterAndStatus)
{
    SCOPED_TRACE(ustr);

    Status s{};

    EXPECT_TRUE(unit->startHeater());

    EXPECT_TRUE(unit->readStatus(s));
    printStatus(s);
    EXPECT_TRUE(s.heater());

    // clearStatus will not clear heater status
    EXPECT_TRUE(unit->clearStatus());
    EXPECT_TRUE(unit->readStatus(s));
    printStatus(s);
    EXPECT_TRUE(s.heater());

    EXPECT_TRUE(unit->stopHeater());
    EXPECT_TRUE(unit->readStatus(s));
    printStatus(s);

    EXPECT_FALSE(s.heater());
}

TEST_P(TestSHT30, SoftReset)
{
    SCOPED_TRACE(ustr);

    // Soft reset is only possible in standby mode.
    EXPECT_FALSE(unit->softReset());

    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    Status s{};
    // After a reset, the heaters are set to a deactivated state as a default
    // condition (*1)
    EXPECT_TRUE(unit->startHeater());

    EXPECT_TRUE(unit->softReset());

    EXPECT_TRUE(unit->readStatus(s));
    EXPECT_FALSE(s.alertPending());
    EXPECT_FALSE(s.heater());  // *1
    EXPECT_FALSE(s.trackingAlertRH());
    EXPECT_FALSE(s.trackingAlert());
    EXPECT_FALSE(s.reset());
    EXPECT_FALSE(s.command());
    EXPECT_FALSE(s.checksum());
}

TEST_P(TestSHT30, GeneralReset)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->startHeater());

    EXPECT_TRUE(unit->generalReset());

    Status s{};
    EXPECT_TRUE(unit->readStatus(s));
    // The ALERT pin will also become active (high) after powerup and after
    // resets
    EXPECT_TRUE(s.alertPending());
    EXPECT_FALSE(s.heater());
    EXPECT_FALSE(s.trackingAlertRH());
    EXPECT_FALSE(s.trackingAlert());
    EXPECT_TRUE(s.reset());
    EXPECT_FALSE(s.command());
    EXPECT_FALSE(s.checksum());
}

TEST_P(TestSHT30, SerialNumber)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    {
        // Read direct [MSB] SNB_3, SNB_2, CRC, SNB_1, SNB_0, CRC [LSB]
        std::array<uint8_t, 6> rbuf{};
        EXPECT_TRUE(unit->readRegister(GET_SERIAL_NUMBER_ENABLE_STRETCH, rbuf.data(), rbuf.size(), 1));
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
