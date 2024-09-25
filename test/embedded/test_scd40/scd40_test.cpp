/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitSCD40/41
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_SCD40.hpp>
#include <unit/unit_SCD41.hpp>
#include <chrono>
#include <iostream>

// #define UNIT_TEST_SCD41

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::scd4x;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestSCD40 : public ComponentTestBase<UnitSCD40, bool> {
protected:
    virtual UnitSCD40* get_instance() override
    {
#if defined(UNIT_TEST_SCD41)
        auto ptr = new m5::unit::UnitSCD41();
#else
        auto ptr = new m5::unit::UnitSCD40();
#endif
        if (ptr) {
            auto ccfg        = ptr->component_config();
            ccfg.stored_size = 3;
            ptr->component_config(ccfg);

            auto cfg           = ptr->config();
            cfg.start_periodic = false;
            ptr->config(cfg);
        }
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

class TestSCD41 : public ComponentTestBase<UnitSCD41, bool> {
protected:
    virtual UnitSCD41* get_instance() override
    {
        auto ptr           = new m5::unit::UnitSCD41(0x62);
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

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD40,
//                          ::testing::Values(false, true));
//  INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD40, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD40, ::testing::Values(false));

#if defined(UNIT_TEST_SCD41)
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD41,
//                          ::testing::Values(false, true));
//  INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD41, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD41, ::testing::Values(false));
#endif

namespace {
// flot t uu int16 (temperature)
constexpr uint16_t float_to_uint16(const float f)
{
    return f * 65536 / 175;
}

struct ModeParams {
    const char* s;
    Mode mode;
    uint32_t tolerance;
};
constexpr ModeParams mode_table[] = {
    //    {"Normal", Mode::Normal, 1},
    {"LowPower", Mode::LowPower, 500},
};

void check_measurement_values(UnitSCD40* u)
{
    EXPECT_NE(u->co2(), 0);
    EXPECT_TRUE(std::isfinite(u->latest().celsius()));
    EXPECT_TRUE(std::isfinite(u->latest().fahrenheit()));
    EXPECT_TRUE(std::isfinite(u->latest().humidity()));
}
}  // namespace

TEST_P(TestSCD40, BasicCommand)
{
    SCOPED_TRACE(ustr);

    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& m : mode_table) {
        SCOPED_TRACE(m.s);
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_TRUE(unit->startPeriodicMeasurement(m.mode));
        // Return False if already started
        EXPECT_FALSE(unit->startPeriodicMeasurement(m.mode));

        EXPECT_TRUE(unit->inPeriodic());

        // These APIs result in an error during periodic detection
        {
            EXPECT_FALSE(unit->writeTemperatureOffset(0));
            float offset{};
            EXPECT_FALSE(unit->readTemperatureOffset(offset));

            EXPECT_FALSE(unit->writeSensorAltitude(0));
            uint16_t altitude{};
            EXPECT_FALSE(unit->readSensorAltitude(altitude));

            int16_t correction{};
            EXPECT_FALSE(unit->performForcedRecalibration(0, correction));

            EXPECT_FALSE(unit->writeAutomaticSelfCalibrationEnabled(true));
            bool enabled{};
            EXPECT_FALSE(unit->readAutomaticSelfCalibrationEnabled(enabled));

            EXPECT_FALSE(unit->startLowPowerPeriodicMeasurement());

            EXPECT_FALSE(unit->writePersistSettings());

            uint64_t sno{};
            EXPECT_FALSE(unit->readSerialNumber(sno));

            bool malfunction{};
            EXPECT_FALSE(unit->performSelfTest(malfunction));

            EXPECT_FALSE(unit->performFactoryReset());

            EXPECT_FALSE(unit->reInit());
        }
        // These APIs can be used during periodic detection
        EXPECT_TRUE(unit->writeAmbientPressure(0.0f));
    }
}

TEST_P(TestSCD40, Periodic)
{
    SCOPED_TRACE(ustr);

    // Empty
    EXPECT_EQ(unit->co2(), 0U);
    EXPECT_FALSE(std::isfinite(unit->temperature()));
    EXPECT_FALSE(std::isfinite(unit->celsius()));
    EXPECT_FALSE(std::isfinite(unit->fahrenheit()));
    EXPECT_FALSE(std::isfinite(unit->humidity()));

    //
    uint32_t idx{};
    for (auto&& m : mode_table) {
        auto s = m5::utility::formatString("Mode:%s", m.s);
        SCOPED_TRACE(s.c_str());

        EXPECT_TRUE(unit->startPeriodicMeasurement(m.mode));
        EXPECT_TRUE(unit->inPeriodic());
        EXPECT_EQ(unit->updatedMillis(), 0);
        test_periodic_measurement(unit.get(), 3, m.tolerance, check_measurement_values);
        EXPECT_TRUE(unit->stopPeriodicMeasurement());

        EXPECT_EQ(unit->available(), 3);
        EXPECT_FALSE(unit->empty());
        EXPECT_TRUE(unit->full());

        if (idx & 1) {
            uint32_t cnt{};
            while (unit->available()) {
                ++cnt;

                EXPECT_NE(unit->co2(), 0U);
                EXPECT_TRUE(std::isfinite(unit->celsius()));
                EXPECT_TRUE(std::isfinite(unit->fahrenheit()));
                EXPECT_TRUE(std::isfinite(unit->humidity()));
                EXPECT_EQ(unit->co2(), unit->oldest().co2());
                EXPECT_FLOAT_EQ(unit->celsius(), unit->oldest().celsius());
                EXPECT_FLOAT_EQ(unit->fahrenheit(), unit->oldest().fahrenheit());
                EXPECT_FLOAT_EQ(unit->humidity(), unit->oldest().humidity());

                unit->discard();
            }
            EXPECT_EQ(cnt, 2);

            EXPECT_EQ(unit->co2(), 0U);
            EXPECT_TRUE(std::isnan(unit->celsius()));
            EXPECT_TRUE(std::isnan(unit->fahrenheit()));
            EXPECT_TRUE(std::isnan(unit->humidity()));
            EXPECT_EQ(unit->available(), 0);
            EXPECT_TRUE(unit->empty());
            EXPECT_FALSE(unit->full());
        } else {
            EXPECT_NE(unit->co2(), 0U);
            EXPECT_TRUE(std::isfinite(unit->celsius()));
            EXPECT_TRUE(std::isfinite(unit->fahrenheit()));
            EXPECT_TRUE(std::isfinite(unit->humidity()));
            EXPECT_EQ(unit->co2(), unit->oldest().co2());
            EXPECT_FLOAT_EQ(unit->celsius(), unit->oldest().celsius());
            EXPECT_FLOAT_EQ(unit->fahrenheit(), unit->oldest().fahrenheit());
            EXPECT_FLOAT_EQ(unit->humidity(), unit->oldest().humidity());

            unit->flush();
            EXPECT_EQ(unit->co2(), 0U);
            EXPECT_TRUE(std::isnan(unit->celsius()));
            EXPECT_TRUE(std::isnan(unit->fahrenheit()));
            EXPECT_TRUE(std::isnan(unit->humidity()));
            EXPECT_EQ(unit->available(), 0);
            EXPECT_TRUE(unit->empty());
            EXPECT_FALSE(unit->full());
        }
        ++idx;
    }
}

TEST_P(TestSCD40, OnChipOutputSignalCompensation)
{
    SCOPED_TRACE(ustr);

    {
        constexpr float OFFSET{1.234f};
        EXPECT_TRUE(unit->writeTemperatureOffset(OFFSET));
        float offset{};
        EXPECT_TRUE(unit->readTemperatureOffset(offset));
        EXPECT_EQ(float_to_uint16(offset), float_to_uint16(OFFSET)) << "offset:" << offset << " OFFSET:" << OFFSET;
    }

    {
        constexpr uint16_t ALTITUDE{3776};
        EXPECT_TRUE(unit->writeSensorAltitude(ALTITUDE));
        uint16_t altitude{};
        EXPECT_TRUE(unit->readSensorAltitude(altitude));
        EXPECT_EQ(altitude, ALTITUDE);
    }
}

TEST_P(TestSCD40, FieldCalibration)
{
    SCOPED_TRACE(ustr);

    {
        int16_t correction{};
        EXPECT_TRUE(unit->performForcedRecalibration(1234, correction));
    }

    {
        EXPECT_TRUE(unit->writeAutomaticSelfCalibrationEnabled(false));
        bool enabled{};
        EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
        EXPECT_FALSE(enabled);

        EXPECT_TRUE(unit->writeAutomaticSelfCalibrationEnabled(true));
        EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
        EXPECT_TRUE(enabled);
    }
}

TEST_P(TestSCD40, AdvancedFeatures)
{
    SCOPED_TRACE(ustr);

    {
        // Read direct [MSB] SNB_3, SNB_2, CRC, SNB_1, SNB_0, CRC [LSB]
        std::array<uint8_t, 9> rbuf{};
        EXPECT_TRUE(unit->readRegister(m5::unit::scd4x::command::GET_SERIAL_NUMBER, rbuf.data(), rbuf.size(), 1));

        // M5_LOGI("%02x%02x%02x%02x%02x%02x", rbuf[0], rbuf[1], rbuf[3],
        // rbuf[4],
        //         rbuf[6], rbuf[7]);

        m5::types::big_uint16_t w0(rbuf[0], rbuf[1]);
        m5::types::big_uint16_t w1(rbuf[3], rbuf[4]);
        m5::types::big_uint16_t w2(rbuf[6], rbuf[7]);
        uint64_t d_sno = (((uint64_t)w0.get()) << 32) | (((uint64_t)w1.get()) << 16) | ((uint64_t)w2.get());

        // M5_LOGI("d_sno[%llX]", d_sno);

        //
        uint64_t sno{};
        char ssno[13]{};
        EXPECT_TRUE(unit->readSerialNumber(sno));
        EXPECT_TRUE(unit->readSerialNumber(ssno));

        // M5_LOGI("s:[%s] uint64:[%x]", ssno, sno);

        EXPECT_EQ(sno, d_sno);

        std::stringstream stream;
        stream << std::uppercase << std::setw(12) << std::hex << std::setfill('0') << sno;
        std::string s(stream.str());
        EXPECT_STREQ(s.c_str(), ssno);
    }

    // Set
    constexpr float OFFSET{1.234f};
    EXPECT_TRUE(unit->writeTemperatureOffset(OFFSET));
    constexpr uint16_t ALTITUDE{3776};
    EXPECT_TRUE(unit->writeSensorAltitude(ALTITUDE));
    EXPECT_TRUE(unit->writeAutomaticSelfCalibrationEnabled(false));

    EXPECT_TRUE(unit->writePersistSettings());  // Save EEPROM

    // Overwrite settings
    EXPECT_TRUE(unit->writeTemperatureOffset(OFFSET * 2));
    EXPECT_TRUE(unit->writeSensorAltitude(ALTITUDE * 2));
    EXPECT_TRUE(unit->writeAutomaticSelfCalibrationEnabled(true));

    float off{};
    uint16_t alt{};
    bool enabled{};

    EXPECT_TRUE(unit->readTemperatureOffset(off));
    EXPECT_TRUE(unit->readSensorAltitude(alt));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
    EXPECT_EQ(float_to_uint16(off), float_to_uint16(OFFSET * 2));
    EXPECT_EQ(alt, ALTITUDE * 2);
    EXPECT_TRUE(enabled);

    EXPECT_TRUE(unit->reInit());  // Load EEPROM

    // Check saved settings
    EXPECT_TRUE(unit->readTemperatureOffset(off));
    EXPECT_TRUE(unit->readSensorAltitude(alt));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
    EXPECT_EQ(float_to_uint16(off), float_to_uint16(OFFSET));
    EXPECT_EQ(alt, ALTITUDE);
    EXPECT_FALSE(enabled);

    bool malfunction{};
    EXPECT_TRUE(unit->performSelfTest(malfunction));

    EXPECT_TRUE(unit->performFactoryReset());  // Reset EEPROM

    EXPECT_TRUE(unit->readTemperatureOffset(off));
    EXPECT_TRUE(unit->readSensorAltitude(alt));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
    EXPECT_NE(float_to_uint16(off), float_to_uint16(OFFSET));
    EXPECT_NE(alt, ALTITUDE);
    EXPECT_TRUE(enabled);
}

#if defined(UNIT_TEST_SCD41)
TEST_P(TestSCD41, LowPowerSingleshot)
{
    SCOPED_TRACE(ustr);

    {
        UnitSCD40::Data d{};
        EXPECT_TRUE(unit->measureSingleshot(d));
        EXPECT_NE(d.co2(), 0);
        EXPECT_TRUE(std::isfinite(d.temperature()));
        EXPECT_TRUE(std::isfinite(d.humidity()));

        EXPECT_FALSE(unit->measureSingleshot(d));
        m5::utility::delay(5000);

        EXPECT_TRUE(unit->measureSingleshot(d));
        EXPECT_NE(d.co2(), 0);
        EXPECT_TRUE(std::isfinite(d.temperature()));
        EXPECT_TRUE(std::isfinite(d.humidity()));
    }

    {
        UnitSCD40::Data d{};
        EXPECT_TRUE(unit->measureSingleshotRHT(d));
        EXPECT_EQ(d.co2(), 0);
        EXPECT_TRUE(std::isfinite(d.temperature()));
        EXPECT_TRUE(std::isfinite(d.humidity()));

        EXPECT_FALSE(unit->measureSingleshotRHT(d));
        m5::utility::delay(50);

        EXPECT_TRUE(unit->measureSingleshotRHT(d));
        EXPECT_EQ(d.co2(), 0);
        EXPECT_TRUE(std::isfinite(d.temperature()));
        EXPECT_TRUE(std::isfinite(d.humidity()));
    }
}
#endif
