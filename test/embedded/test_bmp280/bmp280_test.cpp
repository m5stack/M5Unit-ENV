/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitBMP280
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_BMP280.hpp>
#include <chrono>
#include <cmath>
#include <random>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::bmp280;
using namespace m5::unit::bmp280::command;
using m5::unit::types::elapsed_time_t;

constexpr uint32_t STORED_SIZE{8};

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestBMP280 : public ComponentTestBase<UnitBMP280, bool> {
protected:
    virtual UnitBMP280* get_instance() override
    {
        auto ptr         = new m5::unit::UnitBMP280();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = STORED_SIZE;
        ptr->component_config(ccfg);
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };

    void print_ctrl_measurement(const char* msg = "")
    {
        uint8_t v{};
        unit->readRegister8(CONTROL_MEASUREMENT, v, 0);
        M5_LOGI("%s CM:%02X", msg, v);
    }
    void print_status(const char* msg = "")
    {
        uint8_t v{};
        unit->readRegister8(GET_STATUS, v, 0);
        M5_LOGI("%s S:%02X", msg, v);
    }
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestBMP280,
//                          ::testing::Values(false, true));
//  INSTANTIATE_TEST_SUITE_P(ParamValues, TestBMP280, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestBMP280, ::testing::Values(false));

namespace {

constexpr Oversampling os_table[] = {
    Oversampling::Skipped, Oversampling::X1, Oversampling::X2, Oversampling::X4, Oversampling::X8, Oversampling::X16,
};

constexpr OversamplingSetting oss_table[] = {
    OversamplingSetting::UltraLowPower,       OversamplingSetting::LowPower,
    OversamplingSetting::StandardResolution,  OversamplingSetting::HighResolution,
    OversamplingSetting::UltraHighResolution,
};

constexpr Oversampling osrss_table[][2] = {
    // Pressure, Temperature
    {Oversampling::X1, Oversampling::X1}, {Oversampling::X2, Oversampling::X1},  {Oversampling::X4, Oversampling::X1},
    {Oversampling::X8, Oversampling::X1}, {Oversampling::X16, Oversampling::X2},
};

constexpr Filter filter_table[] = {
    Filter::Off, Filter::Coeff2, Filter::Coeff4, Filter::Coeff8, Filter::Coeff16,

};

constexpr Standby standby_table[] = {
    Standby::Time0_5ms, Standby::Time62_5ms, Standby::Time125ms, Standby::Time250ms,
    Standby::Time500ms, Standby::Time1sec,   Standby::Time2sec,  Standby::Time4sec,
};

constexpr uint32_t standby_time_table[] = {1, 63, 125, 250, 500, 1000, 2000, 4000};

constexpr PowerMode pw_table[] = {
    PowerMode::Sleep,
    PowerMode::Forced,
    PowerMode::Normal,
};

constexpr UseCase uc_table[] = {
    UseCase::LowPower, UseCase::Dynamic, UseCase::Weather, UseCase::Elevator, UseCase::Drop, UseCase::Indoor,
};

struct UseCaseSetting {
    OversamplingSetting osrss;
    Filter filter;
    Standby st;
};
constexpr UseCaseSetting uc_val_table[] = {
    {OversamplingSetting::UltraHighResolution, Filter::Coeff4, Standby::Time62_5ms},
    {OversamplingSetting::StandardResolution, Filter::Coeff16, Standby::Time0_5ms},
    {OversamplingSetting::UltraLowPower, Filter::Off, Standby::Time4sec},
    {OversamplingSetting::StandardResolution, Filter::Coeff4, Standby::Time125ms},
    {OversamplingSetting::LowPower, Filter::Off, Standby::Time0_5ms},
    {OversamplingSetting::UltraHighResolution, Filter::Coeff16, Standby::Time0_5ms},
};

template <class U>
elapsed_time_t test_periodic(U* unit, const uint32_t times, const uint32_t measure_duration = 0)
{
    auto tm         = unit->interval();
    auto timeout_at = m5::utility::millis() + 10 * 1000;

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
        //        m5::utility::delay(1);

    } while (measured < times && m5::utility::millis() <= timeout_at);
    return (measured == times) ? m5::utility::millis() - start_at : 0;

    //   return (measured == times) ? unit->updatedMillis() - start_at : 0;
}

uint32_t calculate_measure_time(const Oversampling osrsP, const Oversampling osrsT, const Filter f)
{
    uint32_t px = ((1U << m5::stl::to_underlying(osrsP) >> 1));
    uint32_t tx = ((1U << m5::stl::to_underlying(osrsT) >> 1));
    // uint32_t fx = (1U << m5::stl::to_underlying(f));
    // if (fx == 1) {
    //     fx = 0;
    // }

    //    M5_LOGI("%u,%u,%u => %u,%u,%u", osrsP, osrsT, f,
    //        px,tx,fx);

    float pt = 2.3f * px;
    float tt = 2.3f * tx;
    //    float ft = 0.5f * fx;
    return pt + tt + 0.5f;
}

}  // namespace

TEST_P(TestBMP280, Settings)
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

TEST_P(TestBMP280, UseCase)
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
        Standby st{};
        const auto& val   = uc_val_table[m5::stl::to_underlying(uc)];
        const auto& osrrs = osrss_table[m5::stl::to_underlying(val.osrss)];

        EXPECT_TRUE(unit->readOversampling(p, t));
        EXPECT_TRUE(unit->readFilter(f));
        EXPECT_TRUE(unit->readStandbyTime(st));

        EXPECT_EQ(p, osrrs[0]);
        EXPECT_EQ(t, osrrs[1]);
        EXPECT_EQ(f, val.filter);
        EXPECT_EQ(st, val.st);
    }
}

TEST_P(TestBMP280, Reset)
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
    EXPECT_NE(s, Standby::Time0_5ms);
    EXPECT_EQ(pm, PowerMode::Normal);

    EXPECT_TRUE(unit->softReset());

    EXPECT_TRUE(unit->readOversampling(p, t));
    EXPECT_TRUE(unit->readFilter(f));
    EXPECT_TRUE(unit->readStandbyTime(s));
    EXPECT_TRUE(unit->readPowerMode(pm));

    EXPECT_EQ(p, Oversampling::Skipped);
    EXPECT_EQ(t, Oversampling::Skipped);
    EXPECT_EQ(f, Filter::Off);
    EXPECT_EQ(s, Standby::Time0_5ms);
    EXPECT_EQ(pm, PowerMode::Sleep);
}

TEST_P(TestBMP280, SingleShot)
{
    SCOPED_TRACE(ustr);

    bmp280::Data discard{};

    // This process fails during periodic measurements.
    EXPECT_TRUE(unit->inPeriodic());
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
                bmp280::Data d{};
                bool can_not_measure  = to == Oversampling::Skipped;
                bool only_temperature = to != Oversampling::Skipped && po == Oversampling::Skipped;

                // M5_LOGI("%s:<%u><%u>", s.c_str(), can_not_measure, only_temperature);

                if (can_not_measure) {
                    EXPECT_FALSE(unit->measureSingleshot(d, po, to, coeff));

                } else if (only_temperature) {
                    EXPECT_TRUE(unit->measureSingleshot(d, po, to, coeff));

                    // M5_LOGI("%f/%f", d.celsius(), d.pressure());

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

TEST_P(TestBMP280, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& uc : uc_table) {
        auto s = m5::utility::formatString("UC:%u", uc);
        SCOPED_TRACE(s);

        const auto& val   = uc_val_table[m5::stl::to_underlying(uc)];
        const auto& osrrs = osrss_table[m5::stl::to_underlying(val.osrss)];
        elapsed_time_t tm{};

        if (val.st == Standby::Time0_5ms) {
            tm = calculate_measure_time(osrrs[0], osrrs[1], val.filter);
        } else {
            tm = standby_time_table[m5::stl::to_underlying(val.st)];
        }

        EXPECT_TRUE(unit->writeUseCaseSetting(uc));
        EXPECT_TRUE(unit->startPeriodicMeasurement());
        EXPECT_TRUE(unit->inPeriodic());

        auto elapsed = test_periodic(unit.get(), STORED_SIZE, tm);

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        // M5_LOGW("E:%ld tm:%ld/%ld", elapsed, tm, tm * STORED_SIZE);

        EXPECT_NE(elapsed, 0);
        EXPECT_LE(elapsed, STORED_SIZE * tm);

        //
        EXPECT_EQ(unit->available(), STORED_SIZE);
        EXPECT_FALSE(unit->empty());
        EXPECT_TRUE(unit->full());

        uint32_t cnt{STORED_SIZE / 2};
        while (cnt-- && unit->available()) {
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
