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
using m5::unit::types::elapsed_time_t;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

constexpr uint32_t STORED_SIZE{8};

class TestQMP6988 : public ComponentTestBase<UnitQMP6988, bool> {
protected:
    virtual UnitQMP6988* get_instance() override
    {
        auto ptr         = new m5::unit::UnitQMP6988();
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

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestQMP6988,
//                          ::testing::Values(false, true));
//  INSTANTIATE_TEST_SUITE_P(ParamValues, TestQMP6988, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestQMP6988, ::testing::Values(false));

namespace {
constexpr Oversampling os_table[] = {
    Oversampling::Skipped, Oversampling::X1,  Oversampling::X2,  Oversampling::X4,
    Oversampling::X8,      Oversampling::X16, Oversampling::X32, Oversampling::X64,
};

constexpr OversamplingSetting oss_table[] = {
    OversamplingSetting::HighSpeed,    OversamplingSetting::LowPower,           OversamplingSetting::Standard,
    OversamplingSetting::HighAccuracy, OversamplingSetting::UltraHightAccuracy,
};

constexpr Oversampling osrss_table[][2] = {
    // Pressure, Temperature
    {Oversampling::X2, Oversampling::X1},  {Oversampling::X4, Oversampling::X1},  {Oversampling::X8, Oversampling::X1},
    {Oversampling::X16, Oversampling::X2}, {Oversampling::X32, Oversampling::X4},
};

constexpr Filter filter_table[] = {
    Filter::Off, Filter::Coeff2, Filter::Coeff4, Filter::Coeff8, Filter::Coeff16, Filter::Coeff32,

};

constexpr Standby standby_table[] = {
    Standby::Time1ms,   Standby::Time5ms,  Standby::Time50ms, Standby::Time250ms,
    Standby::Time500ms, Standby::Time1sec, Standby::Time2sec, Standby::Time4sec,
};

constexpr PowerMode pw_table[] = {
    PowerMode::Sleep,
    PowerMode::Forced,
    PowerMode::Normal,
};

constexpr UseCase uc_table[] = {
    UseCase::Weather, UseCase::Drop, UseCase::Elevator, UseCase::Stair, UseCase::Indoor,
};

struct UseCaseSetting {
    OversamplingSetting osrss;
    Filter filter;
};

constexpr UseCaseSetting uc_val_table[] = {
    {OversamplingSetting::HighSpeed, Filter::Off},
    {OversamplingSetting::LowPower, Filter::Off},
    {OversamplingSetting::Standard, Filter::Coeff4},
    {OversamplingSetting::HighAccuracy, Filter::Coeff8},
    {OversamplingSetting::UltraHightAccuracy, Filter::Coeff32},
};

template <class U>
elapsed_time_t test_periodic(U* unit, const uint32_t times, const uint32_t measure_duration = 0)
{
    auto tm         = unit->interval();
    auto timeout_at = m5::utility::millis() + 8 * 1000;
    // First measured
    do {
        unit->update();
        if (unit->updated()) {
            break;
        }
        std::this_thread::yield();
    } while (!unit->updated() && m5::utility::millis() <= timeout_at);
    // timeout
    if (!unit->updated()) {
        return 0;
    }

    //
    uint32_t measured{};
    auto start_at = m5::utility::millis();
    timeout_at    = start_at + (times * (tm + measure_duration) * 2);
    do {
        unit->update();
        measured += unit->updated() ? 1 : 0;
        if (measured >= times) {
            break;
        }
        std::this_thread::yield();
        // m5::utility::delay(1);

    } while (measured < times && m5::utility::millis() <= timeout_at);

    if (measured == times) {
        return m5::utility::millis() - start_at;
    }
    M5_LOGE("measured:%u", measured);
    return -1;
}

}  // namespace

TEST_P(TestQMP6988, Settings)
{
    SCOPED_TRACE(ustr);

    // Oversampling
    if (1) {
        // This process fails during periodic measurements.
        EXPECT_TRUE(unit->inPeriodic());
        for (auto&& po : os_table) {
            for (auto&& to : os_table) {
                auto s = m5::utility::formatString("OSRS:%u/%u", po, to);
                SCOPED_TRACE(s);

                EXPECT_FALSE(unit->writeOversampling(po, to));
                EXPECT_FALSE(unit->writeOversamplingPressure(po));
                EXPECT_FALSE(unit->writeOversamplingTemperature(to));
            }
        }

        // Success if not in periodic measurement
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());
        for (auto&& po : os_table) {
            for (auto&& to : os_table) {
                Oversampling p;
                Oversampling t;

                auto s = m5::utility::formatString("OSRS:%u/%u", po, to);
                SCOPED_TRACE(s);

                EXPECT_TRUE(unit->writeOversampling(po, to));
                EXPECT_TRUE(unit->readOversampling(p, t));
                EXPECT_EQ(p, po);
                EXPECT_EQ(t, to);

                // Write reverse settings and check
                EXPECT_TRUE(unit->writeOversamplingPressure(to));
                EXPECT_TRUE(unit->readOversampling(p, t));
                EXPECT_EQ(p, to);
                EXPECT_EQ(t, to);

                EXPECT_TRUE(unit->writeOversamplingTemperature(po));
                EXPECT_TRUE(unit->readOversampling(p, t));
                EXPECT_EQ(p, to);
                EXPECT_EQ(t, po);
            }
        }
    }

    // OversamplingSettings
    if (1) {
        // This process fails during periodic measurements.
        EXPECT_TRUE(unit->startPeriodicMeasurement());
        EXPECT_TRUE(unit->inPeriodic());
        for (auto& oss : oss_table) {
            auto s = m5::utility::formatString("OSS:%u", oss);
            SCOPED_TRACE(s);
            EXPECT_FALSE(unit->writeOversampling(oss));
        }

        // Success if not in periodic measurement
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        uint32_t idx{};
        for (auto& oss : oss_table) {
            auto s = m5::utility::formatString("OSS:%u", oss);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->writeOversampling(oss));

            Oversampling p;
            Oversampling t;
            EXPECT_TRUE(unit->readOversampling(p, t));
            EXPECT_EQ(p, osrss_table[idx][0]);
            EXPECT_EQ(t, osrss_table[idx][1]);

            ++idx;
        }
    }

    // Filter
    if (1) {
        // This process fails during periodic measurements.
        EXPECT_TRUE(unit->startPeriodicMeasurement());
        EXPECT_TRUE(unit->inPeriodic());

        for (auto&& e : filter_table) {
            auto s = m5::utility::formatString("F:%u", e);
            SCOPED_TRACE(s);

            EXPECT_FALSE(unit->writeFilter(e));
        }

        // Success if not in periodic measurement
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        for (auto&& e : filter_table) {
            auto s = m5::utility::formatString("F:%u", e);
            SCOPED_TRACE(s);
            EXPECT_TRUE(unit->writeFilter(e));

            Filter f;
            EXPECT_TRUE(unit->readFilter(f));
            EXPECT_EQ(f, e);
        }
    }

    // Standby
    if (1) {
        // This process fails during periodic measurements.
        EXPECT_TRUE(unit->startPeriodicMeasurement());
        EXPECT_TRUE(unit->inPeriodic());
        for (auto&& e : standby_table) {
            auto s = m5::utility::formatString("ST:%u", e);
            SCOPED_TRACE(s);
            EXPECT_FALSE(unit->writeStandbyTime(e));
        }

        // Success if not in periodic measurement
        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());
        for (auto&& e : standby_table) {
            auto s = m5::utility::formatString("ST:%u", e);
            SCOPED_TRACE(s);
            EXPECT_TRUE(unit->writeStandbyTime(e));

            Standby st;
            EXPECT_TRUE(unit->readStandbyTime(st));
            EXPECT_EQ(st, e);
        }
    }

    // PowerMode
    if (1) {
        for (auto&& pw : pw_table) {
            auto s = m5::utility::formatString("PM:%u", pw);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->writePowerMode(pw));

            PowerMode p{};
            EXPECT_TRUE(unit->readPowerMode(p));
            EXPECT_EQ(p, pw);
        }
    }
}

TEST_P(TestQMP6988, UseCase)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->inPeriodic());

    // This process fails during periodic measurements.
    for (auto&& uc : uc_table) {
        auto s = m5::utility::formatString("UC:%u", uc);
        SCOPED_TRACE(s);
        EXPECT_FALSE(unit->writeUseCaseSetting(uc));
    }

    // Success if not in periodic measurement
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
    for (auto&& uc : uc_table) {
        auto s = m5::utility::formatString("UC:%u", uc);
        SCOPED_TRACE(s);
        EXPECT_TRUE(unit->writeUseCaseSetting(uc));

        Oversampling p{};
        Oversampling t{};
        Filter f{};
        const auto& val   = uc_val_table[m5::stl::to_underlying(uc)];
        const auto& osrrs = osrss_table[m5::stl::to_underlying(val.osrss)];

        EXPECT_TRUE(unit->readOversampling(p, t));
        EXPECT_TRUE(unit->readFilter(f));

        EXPECT_EQ(p, osrrs[0]);
        EXPECT_EQ(t, osrrs[1]);
        EXPECT_EQ(f, val.filter);
    }
}

TEST_P(TestQMP6988, Reset)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());

    Oversampling p{}, t{};
    Filter f{};
    Standby s{};
    PowerMode pm{};
    EXPECT_TRUE(unit->readOversampling(p, t));
    EXPECT_TRUE(unit->readFilter(f));
    EXPECT_TRUE(unit->readStandbyTime(s));
    EXPECT_TRUE(unit->readPowerMode(pm));

    EXPECT_NE(p, Oversampling::Skipped);
    EXPECT_NE(t, Oversampling::Skipped);
    EXPECT_NE(f, Filter::Off);
    EXPECT_NE(s, Standby::Time1ms);
    EXPECT_EQ(pm, PowerMode::Normal);

    EXPECT_TRUE(unit->softReset());

    EXPECT_TRUE(unit->readOversampling(p, t));
    EXPECT_TRUE(unit->readFilter(f));
    EXPECT_TRUE(unit->readStandbyTime(s));
    EXPECT_TRUE(unit->readPowerMode(pm));

    EXPECT_EQ(p, Oversampling::Skipped);
    EXPECT_EQ(t, Oversampling::Skipped);
    EXPECT_EQ(f, Filter::Off);
    EXPECT_EQ(s, Standby::Time1ms);
    EXPECT_EQ(pm, PowerMode::Sleep);
}

TEST_P(TestQMP6988, SingleShot)
{
    SCOPED_TRACE(ustr);

    // This process fails during periodic measurements.
    EXPECT_TRUE(unit->inPeriodic());
    qmp6988::Data discard{};
    EXPECT_FALSE(unit->measureSingleshot(discard));

    // Success if not in periodic measurement
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    // Standby time for periodic measurement, does not affect single shots
    EXPECT_TRUE(unit->writeStandbyTime(Standby::Time4sec));

    for (auto&& po : os_table) {
        for (auto&& to : os_table) {
            for (auto&& coeff : filter_table) {
                auto s = m5::utility::formatString("Singleshot OS:%u/%u F:%u", po, to, coeff);
                SCOPED_TRACE(s);

                // Specify settings and measure
                qmp6988::Data d{};
                bool can_not_measure  = to == Oversampling::Skipped;
                bool only_temperature = to != Oversampling::Skipped && po == Oversampling::Skipped;

                // M5_LOGI("%s:<%u><%u>", s.c_str(), can_not_measure, only_temperature);

                if (can_not_measure) {
                    EXPECT_FALSE(unit->measureSingleshot(d, po, to, coeff));

                } else if (only_temperature) {
                    EXPECT_TRUE(unit->measureSingleshot(d, po, to, coeff));

                    // M5_LOGI("%f/%f/%f", d.celsius(), d.fahrenheit(), d.pressure());

                    EXPECT_TRUE(std::isfinite(d.celsius()));
                    EXPECT_TRUE(std::isfinite(d.fahrenheit()));
                    EXPECT_FALSE(std::isfinite(d.pressure()));
                } else {
                    EXPECT_TRUE(unit->measureSingleshot(d, po, to, coeff));

                    // M5_LOGI("%f/%f", d.celsius(), d.pressure());

                    EXPECT_TRUE(std::isfinite(d.celsius()));
                    EXPECT_TRUE(std::isfinite(d.fahrenheit()));
                    EXPECT_TRUE(std::isfinite(d.pressure()));
                }

                if (!can_not_measure) {
                    Oversampling t;
                    Oversampling p;
                    Filter f;
                    EXPECT_TRUE(unit->readOversampling(p, t));
                    EXPECT_TRUE(unit->readFilter(f));
                    EXPECT_EQ(p, po);
                    EXPECT_EQ(t, to);
                    EXPECT_EQ(f, coeff);
                }
            }
        }
    }
}

TEST_P(TestQMP6988, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& uc : uc_table) {
        for (auto&& st : standby_table) {
            auto s = m5::utility::formatString("UC:%u ST:%u", uc, st);
            SCOPED_TRACE(s);

            // M5_LOGW("%s", s.c_str());

            const auto& val   = uc_val_table[m5::stl::to_underlying(uc)];
            const auto& osrrs = osrss_table[m5::stl::to_underlying(val.osrss)];

            EXPECT_TRUE(unit->writeUseCaseSetting(uc));
            EXPECT_TRUE(unit->writeStandbyTime(st));
            EXPECT_TRUE(unit->startPeriodicMeasurement());
            EXPECT_TRUE(unit->inPeriodic());
            auto tm      = unit->interval();
            auto elapsed = test_periodic(unit.get(), STORED_SIZE, (int)st == 0 ? ((uint32_t)uc + 1) * 2 : 0);

            EXPECT_TRUE(unit->stopPeriodicMeasurement());
            EXPECT_FALSE(unit->inPeriodic());

            // M5_LOGW("E:(%u) %ld", tm == 1 ? (uint32_t)uc * 2 : 0, elapsed);

            EXPECT_NE(elapsed, 0);
            EXPECT_NE(elapsed, -1);
            EXPECT_GE(elapsed, STORED_SIZE * tm - 1);

            Oversampling t;
            Oversampling p;
            Filter f;
            EXPECT_TRUE(unit->readOversampling(p, t));
            EXPECT_TRUE(unit->readFilter(f));
            EXPECT_EQ(p, osrrs[0]);
            EXPECT_EQ(t, osrrs[1]);
            EXPECT_EQ(f, val.filter);

            //
            EXPECT_EQ(unit->available(), STORED_SIZE);
            EXPECT_FALSE(unit->empty());
            EXPECT_TRUE(unit->full());

            uint32_t cnt{STORED_SIZE / 2};
            while (cnt-- && unit->available()) {
                // M5_LOGI("%f/%f/%f", unit->celsius(), unit->fahrenheit(), unit->pressure());

                EXPECT_TRUE(std::isfinite(unit->temperature()));
                EXPECT_TRUE(std::isfinite(unit->fahrenheit()));
                EXPECT_TRUE(std::isfinite(unit->pressure()));
                EXPECT_FLOAT_EQ(unit->temperature(), unit->oldest().temperature());
                EXPECT_FLOAT_EQ(unit->pressure(), unit->oldest().pressure());
                EXPECT_FALSE(unit->empty());
                unit->discard();
            }
            EXPECT_EQ(unit->available(), STORED_SIZE / 2);
            EXPECT_FALSE(unit->empty());
            EXPECT_FALSE(unit->full());

            unit->flush();
            EXPECT_EQ(unit->available(), 0);
            EXPECT_TRUE(unit->empty());
            EXPECT_FALSE(unit->full());

            EXPECT_FALSE(std::isfinite(unit->temperature()));
            EXPECT_FALSE(std::isfinite(unit->pressure()));
        }
    }
}
