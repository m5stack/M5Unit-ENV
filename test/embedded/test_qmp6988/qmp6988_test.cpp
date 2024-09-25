/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitQMP6988
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_QMP6988.hpp>
#include <chrono>
#include <cmath>
#include <random>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::qmp6988;
using namespace m5::unit::qmp6988::command;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestQMP6988 : public ComponentTestBase<UnitQMP6988, bool> {
protected:
    virtual UnitQMP6988* get_instance() override
    {
        auto ptr         = new m5::unit::UnitQMP6988();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = 2;
        ptr->component_config(ccfg);
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestQMP6988,
//                          ::testing::Values(false, true));
//  INSTANTIATE_TEST_SUITE_P(ParamValues, TestQMP6988, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestQMP6988, ::testing::Values(false));

namespace {
// flot t uu int16 (temperature)
constexpr uint16_t float_to_uint16(const float f)
{
    return f * 65536 / 175;
}

constexpr Filter filter_table[] = {
    Filter::Off, Filter::Coeff2, Filter::Coeff4, Filter::Coeff8, Filter::Coeff16, Filter::Coeff32,

};

constexpr Standby standby_table[] = {
    Standby::Time1ms,   Standby::Time5ms,  Standby::Time50ms, Standby::Time250ms,
    Standby::Time500ms, Standby::Time1sec, Standby::Time2sec, Standby::Time4sec,
};

constexpr Oversampling os_table[] = {
    Oversampling::Skip, Oversampling::X1,  Oversampling::X2,  Oversampling::X4,
    Oversampling::X8,   Oversampling::X16, Oversampling::X32, Oversampling::X64,
};

void check_measurement_values(UnitQMP6988* u)
{
    EXPECT_TRUE(std::isfinite(u->latest().celsius()));
    EXPECT_TRUE(std::isfinite(u->latest().fahrenheit()));
    EXPECT_TRUE(std::isfinite(u->latest().pressure()));
}

}  // namespace

TEST_P(TestQMP6988, MeasurementCondition)
{
    SCOPED_TRACE(ustr);

    // This process fails during periodic measurements.
    for (auto&& ta : os_table) {
        for (auto&& pa : os_table) {
            auto s = m5::utility::formatString("OS:%u/%u", ta, pa);
            SCOPED_TRACE(s);

            EXPECT_FALSE(unit->writeOversamplings(ta, pa));
            EXPECT_FALSE(unit->writeOversamplingTemperature(ta));
            EXPECT_FALSE(unit->writeOversamplingPressure(pa));
        }
    }

    // Success if not in periodic measurement
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    for (auto&& ta : os_table) {
        for (auto&& pa : os_table) {
            Oversampling t;
            Oversampling p;

            auto s = m5::utility::formatString("OS:%u/%u", ta, pa);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->writeOversamplings(ta, pa));
            EXPECT_TRUE(unit->readOversamplings(t, p));
            EXPECT_EQ(t, ta);
            EXPECT_EQ(p, pa);

            EXPECT_TRUE(unit->writeOversamplingTemperature(pa));
            EXPECT_TRUE(unit->readOversamplings(t, p));
            EXPECT_EQ(t, pa);
            EXPECT_EQ(p, pa);

            EXPECT_TRUE(unit->writeOversamplingPressure(ta));
            EXPECT_TRUE(unit->readOversamplings(t, p));
            EXPECT_EQ(t, pa);
            EXPECT_EQ(p, ta);
        }
    }
}

TEST_P(TestQMP6988, IIRFilter)
{
    SCOPED_TRACE(ustr);

    // This process fails during periodic measurements.
    for (auto&& e : filter_table) {
        EXPECT_FALSE(unit->writeFilterCoeff(e));
    }

    // Success if not in periodic measurement
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    for (auto&& e : filter_table) {
        EXPECT_TRUE(unit->writeFilterCoeff(e));

        Filter f;
        EXPECT_TRUE(unit->readFilterCoeff(f));
        EXPECT_EQ(f, e);
    }
}

TEST_P(TestQMP6988, PowerMode)
{
    SCOPED_TRACE(ustr);
}

TEST_P(TestQMP6988, UseCase)
{
    SCOPED_TRACE(ustr);

    Filter f;
    Oversampling t;
    Oversampling p;

    // This process fails during periodic measurements.
    EXPECT_FALSE(unit->writeWeathermonitoringSetting());
    EXPECT_FALSE(unit->writeDropDetectionSetting());
    EXPECT_FALSE(unit->writeElevatorDetectionSetting());
    EXPECT_FALSE(unit->writeStairDetectionSetting());
    EXPECT_FALSE(unit->writeIndoorNavigationSetting());

    // Success if not in periodic measurement
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    {
        SCOPED_TRACE("Weather");
        EXPECT_TRUE(unit->writeWeathermonitoringSetting());

        EXPECT_TRUE(unit->readFilterCoeff(f));
        EXPECT_TRUE(unit->readOversamplings(t, p));
        EXPECT_EQ(t, Oversampling::X2);
        EXPECT_EQ(p, Oversampling::X1);
        EXPECT_EQ(f, Filter::Off);
    }

    {
        SCOPED_TRACE("Drop");
        EXPECT_TRUE(unit->writeDropDetectionSetting());

        EXPECT_TRUE(unit->readFilterCoeff(f));
        EXPECT_TRUE(unit->readOversamplings(t, p));
        EXPECT_EQ(t, Oversampling::X4);
        EXPECT_EQ(p, Oversampling::X1);
        EXPECT_EQ(f, Filter::Off);
    }

    {
        SCOPED_TRACE("Elevator");
        EXPECT_TRUE(unit->writeElevatorDetectionSetting());

        EXPECT_TRUE(unit->readFilterCoeff(f));
        EXPECT_TRUE(unit->readOversamplings(t, p));
        EXPECT_EQ(t, Oversampling::X8);
        EXPECT_EQ(p, Oversampling::X1);
        EXPECT_EQ(f, Filter::Coeff4);
    }

    {
        SCOPED_TRACE("Stair");
        EXPECT_TRUE(unit->writeStairDetectionSetting());

        EXPECT_TRUE(unit->readFilterCoeff(f));
        EXPECT_TRUE(unit->readOversamplings(t, p));
        EXPECT_EQ(t, Oversampling::X16);
        EXPECT_EQ(p, Oversampling::X2);
        EXPECT_EQ(f, Filter::Coeff8);
    }

    {
        SCOPED_TRACE("Indoor");
        EXPECT_TRUE(unit->writeIndoorNavigationSetting());

        EXPECT_TRUE(unit->readFilterCoeff(f));
        EXPECT_TRUE(unit->readOversamplings(t, p));
        EXPECT_EQ(t, Oversampling::X32);
        EXPECT_EQ(p, Oversampling::X4);
        EXPECT_EQ(f, Filter::Coeff32);
    }
}

TEST_P(TestQMP6988, Setup)
{
    SCOPED_TRACE(ustr);

    // This process fails during periodic measurements.
    for (auto&& e : standby_table) {
        EXPECT_FALSE(unit->writeStandbyTime(e));
    }

    // Success if not in periodic measurement
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    for (auto&& e : standby_table) {
        EXPECT_TRUE(unit->writeStandbyTime(e));

        Standby st;
        EXPECT_TRUE(unit->readStandbyTime(st));
        EXPECT_EQ(st, e);
    }
}

TEST_P(TestQMP6988, Status)
{
    SCOPED_TRACE(ustr);

    Status s;
    EXPECT_TRUE(unit->readStatus(s));
    //    M5_LOGI("Measure:%d, OTP:%d", s.measure(), s.OTP());
}

TEST_P(TestQMP6988, SingleShot)
{
    SCOPED_TRACE(ustr);

    qmp6988::Data d{};
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->measureSingleshot(d));  // inPeriodic
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    // Standby time for periodic measurement does not affect single shots
    EXPECT_TRUE(unit->writeStandbyTime(Standby::Time4sec));

    for (auto&& ta : os_table) {
        for (auto&& pa : os_table) {
            for (auto&& coef : filter_table) {
                auto s = m5::utility::formatString("Singleshot OS:%u/%u F:%u", ta, pa, coef);
                SCOPED_TRACE(s);

                // Specify settings and measure
                qmp6988::Data d{};
                bool not_measure = ta == Oversampling::Skip && pa == Oversampling::Skip;
                if (not_measure) {
                    EXPECT_FALSE(unit->measureSingleshot(d, ta, pa, coef));

                } else {
                    EXPECT_TRUE(unit->measureSingleshot(d, ta, pa, coef));
                    EXPECT_TRUE(std::isfinite(d.celsius()));
                    EXPECT_TRUE(std::isfinite(d.fahrenheit()));
                    EXPECT_TRUE(std::isfinite(d.pressure()));

                    // M5_LOGI("%s> T:%f/%f P:%f", s.c_str(), d.celsius(),
                    //         d.fahrenheit(), d.pressure());
                }

                Oversampling t;
                Oversampling p;
                Filter f;
                EXPECT_TRUE(unit->readOversamplings(t, p));
                EXPECT_TRUE(unit->readFilterCoeff(f));
                EXPECT_EQ(t, ta);
                EXPECT_EQ(p, pa);
                EXPECT_EQ(f, coef);
            }
        }
    }
}

// #define TEST_ALL_COMBINATIONS

TEST_P(TestQMP6988, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

#if defined(TEST_ALL_COMBINATIONS)
    for (auto&& ta : os_table) {
        for (auto&& pa : os_table) {
            for (auto&& coef : filter_table) {
                for (auto&& st : standby_table) {
                    auto s = m5::utility::formatString("Periodic OS:%u/%u F:%u ST:%u", ta, pa, coef, st);
                    SCOPED_TRACE(s);

                    EXPECT_TRUE(unit->startPeriodicMeasurement(st));
                    EXPECT_TRUE(unit->inPeriodic());
                    EXPECT_EQ(unit->updatedMillis(), 0);

                    test_periodic_measurement(unit.get(), 4, 1, check_measurement_values);

                    EXPECT_TRUE(unit->stopPeriodicMeasurement());
                    EXPECT_FALSE(unit->inPeriodic());

                    EXPECT_EQ(unit->available(), 2);
                    EXPECT_TRUE(unit->full());
                    EXPECT_FALSE(unit->empty());
                    Standby ss;
                    EXPECT_TRUE(unit->readStandbyTime(ss));
                    EXPECT_EQ(ss, st);

                    uint32_t cnt{};
                    while (unit->available()) {
                        ++cnt;
                        EXPECT_FALSE(unit->empty());

                        // M5_LOGI("T:%f/%f P:%f", unit->celsius(),
                        // unit->fahrenheit(),
                        //         unit->pressure());

                        EXPECT_TRUE(std::isfinite(unit->temperature()));
                        EXPECT_TRUE(std::isfinite(unit->pressure()));
                        EXPECT_FLOAT_EQ(unit->temperature(), unit->oldest().temperature());
                        EXPECT_FLOAT_EQ(unit->pressure(), unit->oldest().pressure());
                        unit->discard();
                    }

                    EXPECT_EQ(cnt, 2);
                    EXPECT_TRUE(std::isnan(unit->temperature()));
                    EXPECT_TRUE(std::isnan(unit->pressure()));
                    EXPECT_EQ(unit->available(), 0);
                    EXPECT_TRUE(unit->empty());
                    EXPECT_FALSE(unit->full());
                }
            }
        }
    }
#else

    auto rng = std::default_random_engine{};
    std::uniform_int_distribution<> dist_os(1, 7);
    std::uniform_int_distribution<> dist_f(0, 5);

    uint32_t idx{};
    for (auto&& st : standby_table) {
        Oversampling ost = static_cast<Oversampling>(dist_os(rng));
        Oversampling osp = static_cast<Oversampling>(dist_os(rng));
        Filter f         = static_cast<Filter>(dist_f(rng));

        auto s = m5::utility::formatString("Periodic ST:%u OST:%u OSP:%u F:%u", st, ost, osp, f);
        SCOPED_TRACE(s);
        // M5_LOGI("%s", s.c_str());

        EXPECT_TRUE(unit->startPeriodicMeasurement(st, ost, osp, f));
        EXPECT_TRUE(unit->inPeriodic());
        EXPECT_EQ(unit->updatedMillis(), 0);

#if 1
        test_periodic_measurement(unit.get(), 4, 1, check_measurement_values);

#else
        auto avg = test_periodic_measurement(unit.get(), idx == 0 ? 100 : 10, 1, check_measurement_values);
        M5_LOGW("%s>I:%lu A:%u", s.c_str(), unit->interval(), avg);
#endif

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_EQ(unit->available(), 2);
        EXPECT_TRUE(unit->full());
        EXPECT_FALSE(unit->empty());
        Standby ss;
        EXPECT_TRUE(unit->readStandbyTime(ss));
        EXPECT_EQ(ss, st);

        if (idx & 1) {
            uint32_t cnt{};
            while (unit->available()) {
                ++cnt;
                EXPECT_FALSE(unit->empty());

                // M5_LOGI("T:%f/%f P:%f", unit->celsius(), unit->fahrenheit(),
                //         unit->pressure());

                EXPECT_TRUE(std::isfinite(unit->temperature()));
                EXPECT_TRUE(std::isfinite(unit->pressure()));
                EXPECT_FLOAT_EQ(unit->temperature(), unit->oldest().temperature());
                EXPECT_FLOAT_EQ(unit->pressure(), unit->oldest().pressure());
                unit->discard();
            }

            EXPECT_EQ(cnt, 2);
            EXPECT_TRUE(std::isnan(unit->temperature()));
            EXPECT_TRUE(std::isnan(unit->pressure()));
            EXPECT_EQ(unit->available(), 0);
            EXPECT_TRUE(unit->empty());
            EXPECT_FALSE(unit->full());
        } else {
            EXPECT_TRUE(std::isfinite(unit->temperature()));
            EXPECT_TRUE(std::isfinite(unit->pressure()));
            unit->flush();
            EXPECT_TRUE(std::isnan(unit->temperature()));
            EXPECT_TRUE(std::isnan(unit->pressure()));
            EXPECT_EQ(unit->available(), 0);
            EXPECT_TRUE(unit->empty());
            EXPECT_FALSE(unit->full());
        }
        ++idx;
    }

#endif
}
