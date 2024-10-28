/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitBME688
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_helper.hpp>
#include <googletest/test_template.hpp>
#include <unit/unit_BME688.hpp>
#include <chrono>
#include <random>
#include <set>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::bme688;
#if defined(UNIT_BME688_USING_BSEC2)
using namespace m5::unit::bme688::bsec2;
#endif

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestBME688 : public ComponentTestBase<UnitBME688, bool> {
protected:
    virtual UnitBME688* get_instance() override
    {
        auto ptr         = new m5::unit::UnitBME688();
        auto ccfg        = ptr->component_config();
        ccfg.stored_size = 8;
        ptr->component_config(ccfg);
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestBME688,
//                         ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestBME688, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestBME688, ::testing::Values(false));

namespace {
constexpr Oversampling os_table[] = {
    Oversampling::None, Oversampling::x1, Oversampling::x1,  Oversampling::x2,
    Oversampling::x4,   Oversampling::x8, Oversampling::x16,
};
constexpr Filter filter_table[] = {
    Filter::None,     Filter::Coeff_1,  Filter::Coeff_3,  Filter::Coeff_7,
    Filter::Coeff_15, Filter::Coeff_31, Filter::Coeff_63, Filter::Coeff_127,
};

#if defined(UNIT_BME688_USING_BSEC2)
// All outputs
constexpr bsec_virtual_sensor_t vs_table[] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_GAS_PERCENTAGE,
    BSEC_OUTPUT_GAS_ESTIMATE_1,
    BSEC_OUTPUT_GAS_ESTIMATE_2,
    BSEC_OUTPUT_GAS_ESTIMATE_3,
    BSEC_OUTPUT_GAS_ESTIMATE_4,
    BSEC_OUTPUT_RAW_GAS_INDEX,
    BSEC_OUTPUT_REGRESSION_ESTIMATE_1,
    BSEC_OUTPUT_REGRESSION_ESTIMATE_2,
    BSEC_OUTPUT_REGRESSION_ESTIMATE_3,
    BSEC_OUTPUT_REGRESSION_ESTIMATE_4,
};
// Using BSEC2 library configuration files
constexpr uint8_t bsec_config[] = {
#include <config/bme688/bme688_sel_33v_300s_4d/bsec_selectivity.txt>
};
#endif

std::random_device rng;

void check_measurement_values(UnitBME688* u)
{
    auto latest = u->latest();
    // for raw
    EXPECT_TRUE(std::isfinite(latest.raw_temperature()));
    EXPECT_TRUE(std::isfinite(latest.raw_pressure()));
    EXPECT_TRUE(std::isfinite(latest.raw_humidity()));
    EXPECT_TRUE(std::isfinite(latest.raw_gas()));
    // M5_LOGI("%f/%f/%f/%f", latest.raw_temperature(), latest.raw_pressure(), latest.raw_humidity(), latest.raw_gas());
}

}  // namespace

TEST_P(TestBME688, Misc)
{
#if defined(UNIT_BME688_USING_BSEC2)
    for (auto&& v : vs_table) {
        EXPECT_EQ(subscribe_to_bits(v), 1U << v);
    }
    auto bits = subscribe_to_bits(
        BSEC_OUTPUT_IAQ, BSEC_OUTPUT_STATIC_IAQ, BSEC_OUTPUT_CO2_EQUIVALENT, BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_RAW_TEMPERATURE, BSEC_OUTPUT_RAW_PRESSURE, BSEC_OUTPUT_RAW_HUMIDITY, BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_STABILIZATION_STATUS, BSEC_OUTPUT_RUN_IN_STATUS, BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY, BSEC_OUTPUT_GAS_PERCENTAGE, BSEC_OUTPUT_GAS_ESTIMATE_1,
        BSEC_OUTPUT_GAS_ESTIMATE_2, BSEC_OUTPUT_GAS_ESTIMATE_3, BSEC_OUTPUT_GAS_ESTIMATE_4, BSEC_OUTPUT_RAW_GAS_INDEX,
        BSEC_OUTPUT_REGRESSION_ESTIMATE_1, BSEC_OUTPUT_REGRESSION_ESTIMATE_2, BSEC_OUTPUT_REGRESSION_ESTIMATE_3,
        BSEC_OUTPUT_REGRESSION_ESTIMATE_4);
    constexpr uint32_t val{
        1U << BSEC_OUTPUT_IAQ | 1U << BSEC_OUTPUT_STATIC_IAQ | 1U << BSEC_OUTPUT_CO2_EQUIVALENT |
        1U << BSEC_OUTPUT_BREATH_VOC_EQUIVALENT | 1U << BSEC_OUTPUT_RAW_TEMPERATURE | 1U << BSEC_OUTPUT_RAW_PRESSURE |
        1U << BSEC_OUTPUT_RAW_HUMIDITY | 1U << BSEC_OUTPUT_RAW_GAS | 1U << BSEC_OUTPUT_STABILIZATION_STATUS |
        1U << BSEC_OUTPUT_RUN_IN_STATUS | 1U << BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE |
        1U << BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY | 1U << BSEC_OUTPUT_GAS_PERCENTAGE |
        1U << BSEC_OUTPUT_GAS_ESTIMATE_1 | 1U << BSEC_OUTPUT_GAS_ESTIMATE_2 | 1U << BSEC_OUTPUT_GAS_ESTIMATE_3 |
        1U << BSEC_OUTPUT_GAS_ESTIMATE_4 | 1U << BSEC_OUTPUT_RAW_GAS_INDEX | 1U << BSEC_OUTPUT_REGRESSION_ESTIMATE_1 |
        1U << BSEC_OUTPUT_REGRESSION_ESTIMATE_2 | 1U << BSEC_OUTPUT_REGRESSION_ESTIMATE_3 |
        1U << BSEC_OUTPUT_REGRESSION_ESTIMATE_4};
    EXPECT_EQ(bits, val);
#endif
}

TEST_P(TestBME688, Settings)
{
    SCOPED_TRACE(ustr);

    Oversampling os{};
    Filter f{};

    uint32_t serial{};
    EXPECT_TRUE(unit->readUniqueID(serial));
    EXPECT_NE(serial, 0U);

    // TPH
    for (auto&& e : os_table) {
        EXPECT_TRUE(unit->writeOversamplingTemperature(e));
        EXPECT_EQ(unit->tphSetting().os_temp, m5::stl::to_underlying(e));
        EXPECT_TRUE(unit->readOversamplingTemperature(os));
        EXPECT_EQ(os, e);
    }
    for (auto&& e : os_table) {
        EXPECT_TRUE(unit->writeOversamplingPressure(e));
        EXPECT_EQ(unit->tphSetting().os_pres, m5::stl::to_underlying(e));
        EXPECT_TRUE(unit->readOversamplingPressure(os));
        EXPECT_EQ(os, e);
    }
    for (auto&& e : os_table) {
        EXPECT_TRUE(unit->writeOversamplingHumidity(e));
        EXPECT_EQ(unit->tphSetting().os_hum, m5::stl::to_underlying(e));
        EXPECT_TRUE(unit->readOversamplingHumidity(os));
        EXPECT_EQ(os, e);
    }
    for (auto&& e : filter_table) {
        EXPECT_TRUE(unit->writeIIRFilter(e));
        EXPECT_EQ(unit->tphSetting().filter, m5::stl::to_underlying(e));
        EXPECT_TRUE(unit->readIIRFilter(f));
        EXPECT_EQ(f, e);
    }

    uint32_t cnt{10};
    while (cnt--) {
        bme68xConf tph = unit->tphSetting();
        tph.os_temp    = rng() % 0x06;
        tph.os_pres    = rng() % 0x06;
        tph.os_hum     = rng() % 0x06;
        tph.filter     = rng() % 0x07;

        // M5_LOGW("%u/%u/%u/%u", tph.os_temp, tph.os_pres, tph.os_hum,
        //         tph.filter);

        EXPECT_TRUE(unit->writeTPHSetting(tph));
        EXPECT_EQ(unit->tphSetting().os_temp, tph.os_temp);
        EXPECT_EQ(unit->tphSetting().os_pres, tph.os_pres);
        EXPECT_EQ(unit->tphSetting().os_hum, tph.os_hum);

        bme68xConf after{};
        EXPECT_TRUE(unit->readTPHSetting(after));
        EXPECT_TRUE(memcmp(&tph, &after, sizeof(tph)) == 0)
            << tph.os_temp << "/" << tph.os_pres << "/" << tph.os_hum << "/" << tph.filter;

        EXPECT_TRUE(
            unit->writeOversampling((Oversampling)tph.os_temp, (Oversampling)tph.os_pres, (Oversampling)tph.os_hum));
        EXPECT_EQ(unit->tphSetting().os_temp, tph.os_temp);
        EXPECT_EQ(unit->tphSetting().os_pres, tph.os_pres);
        EXPECT_EQ(unit->tphSetting().os_hum, tph.os_hum);

        EXPECT_TRUE(unit->readTPHSetting(after));
        EXPECT_TRUE(memcmp(&tph, &after, sizeof(tph)) == 0)
            << tph.os_temp << "/" << tph.os_pres << "/" << tph.os_hum << "/" << tph.filter;
    }

    // Calibration
    bme68xCalibration c0{}, c1{};
    EXPECT_TRUE(unit->readCalibration(c0));
    EXPECT_TRUE(unit->writeCalibration(c0));
    EXPECT_TRUE(unit->readCalibration(c1));
    EXPECT_TRUE(memcmp(&c0, &c1, sizeof(c1)) == 0);

    // softReset rewinds settings
    EXPECT_TRUE(unit->softReset());

    //
    EXPECT_TRUE(unit->readOversamplingTemperature(os));
    EXPECT_EQ(os, Oversampling::None);
    EXPECT_TRUE(unit->readOversamplingPressure(os));
    EXPECT_EQ(os, Oversampling::None);
    EXPECT_TRUE(unit->readOversamplingHumidity(os));
    EXPECT_EQ(os, Oversampling::None);
    EXPECT_TRUE(unit->readIIRFilter(f));
    EXPECT_EQ(f, Filter::None);
}

#if defined(UNIT_BME688_USING_BSEC2)
TEST_P(TestBME688, BSEC2)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    uint8_t cfg[BSEC_MAX_PROPERTY_BLOB_SIZE]{};
    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE]{};
    uint8_t state2[BSEC_MAX_STATE_BLOB_SIZE]{};
    uint32_t actual{};

    EXPECT_TRUE(unit->bsec2GetState(state, actual));  // get current

    // Version
    auto& ver = unit->bsec2Version();
    EXPECT_NE(ver.major, 0);
    EXPECT_NE(ver.minor, 0);
    // M5_LOGI("bsec2 ver:%u.%u.%u.%u", ver.major, ver.minor, ver.major_bugfix, ver.minor_bugfix);

    // Subscribe
    constexpr bsec_virtual_sensor_t sensorList[] = {
        BSEC_OUTPUT_IAQ,     BSEC_OUTPUT_RAW_TEMPERATURE,      BSEC_OUTPUT_RAW_PRESSURE, BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS, BSEC_OUTPUT_STABILIZATION_STATUS, BSEC_OUTPUT_RUN_IN_STATUS};
    std::vector<bsec_virtual_sensor_t> nosubscribed(vs_table, vs_table + m5::stl::size(vs_table));
    auto it = std::remove_if(nosubscribed.begin(), nosubscribed.end(), [&sensorList](bsec_virtual_sensor_t vs) {
        auto e = std::begin(sensorList) + m5::stl::size(sensorList);
        return std::find(std::begin(sensorList), e, vs) != e;
    });
    nosubscribed.erase(it, nosubscribed.end());

    EXPECT_TRUE(unit->bsec2UpdateSubscription(sensorList, m5::stl::size(sensorList), bsec2::SampleRate::LowPower));
    for (auto&& e : sensorList) {
        EXPECT_TRUE(unit->bsec2IsSubscribed(e)) << e;
    }
    for (auto&& e : nosubscribed) {
        EXPECT_FALSE(unit->bsec2IsSubscribed(e)) << e;
    }

    for (auto&& e : sensorList) {
        EXPECT_TRUE(unit->bsec2Unsubscribe(e)) << e;
        EXPECT_FALSE(unit->bsec2IsSubscribed(e)) << e;
    }
    for (auto&& e : nosubscribed) {
        EXPECT_FALSE(unit->bsec2IsSubscribed(e)) << e;
    }

    for (auto&& e : sensorList) {
        EXPECT_TRUE(unit->bsec2Subscribe(e)) << e;
        EXPECT_TRUE(unit->bsec2IsSubscribed(e)) << e;
    }
    for (auto&& e : nosubscribed) {
        EXPECT_FALSE(unit->bsec2IsSubscribed(e)) << e;
    }

    EXPECT_TRUE(unit->bsec2UnsubscribeAll());  // Same as stop
    for (auto&& e : vs_table) {
        EXPECT_FALSE(unit->bsec2IsSubscribed(e)) << e;
    }

    // Measurement
    EXPECT_TRUE(unit->startPeriodicMeasurement(sensorList, m5::stl::size(sensorList), bsec2::SampleRate::LowPower));
    auto bits = virtual_sensor_array_to_bits(sensorList, m5::stl::size(sensorList));
    EXPECT_EQ(unit->bsec2Subscription(), bits);

#if 1
    test_periodic_measurement(unit.get(), 8, 8, (unit->interval() * 2) * 8, check_measurement_values, false);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_EQ(unit->mode(), Mode::Sleep);

    EXPECT_EQ(unit->available(), 8U);
    EXPECT_FALSE(unit->empty());
    EXPECT_TRUE(unit->full());

    uint32_t cnt{4};
    while (unit->available() && cnt--) {
        EXPECT_TRUE(std::isfinite(unit->iaq()));
        EXPECT_TRUE(std::isfinite(unit->temperature()));
        EXPECT_TRUE(std::isfinite(unit->pressure()));
        EXPECT_TRUE(std::isfinite(unit->humidity()));
        EXPECT_TRUE(std::isfinite(unit->gas()));

        EXPECT_FLOAT_EQ(unit->iaq(), unit->oldest().iaq());
        EXPECT_FLOAT_EQ(unit->temperature(), unit->oldest().temperature());
        EXPECT_FLOAT_EQ(unit->pressure(), unit->oldest().pressure());
        EXPECT_FLOAT_EQ(unit->humidity(), unit->oldest().humidity());
        EXPECT_FLOAT_EQ(unit->gas(), unit->oldest().gas());

        EXPECT_FALSE(unit->empty());
        unit->discard();
    }

#else
    uint32_t cnt{8};
    while (cnt--) {
        auto now{m5::utility::millis()};
        auto timeout_at = now + 3 * 1000;  // LowPower
        do {
            unit->update();
            now = m5::utility::millis();
            if (unit->updated()) {
                break;
            }
            m5::utility::delay(1);
        } while (now <= timeout_at);

        // M5_LOGW("now:%ld timeout_at:%ld", now, timeout_at);

        EXPECT_TRUE(unit->updated());
        if (cnt < 2) {
            EXPECT_LE(now, timeout_at);
        }

        auto d = unit->latest();
        for (auto&& vs : sensorList) {
            float f = d.get(vs);
            // M5_LOGI("[%u]:%.2f", vs, f);
            EXPECT_TRUE(std::isfinite(f)) << vs;
        }
        for (auto&& vs : nosubscribed) {
            float f = d.get(vs);
            EXPECT_FALSE(std::isfinite(f)) << vs;
        }
    }
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_EQ(unit->available(), 8U);
    EXPECT_FALSE(unit->empty());
    EXPECT_TRUE(unit->full());

    cnt = 4;
    while (unit->available() && cnt--) {
        EXPECT_TRUE(std::isfinite(unit->iaq()));
        EXPECT_TRUE(std::isfinite(unit->temperature()));
        EXPECT_TRUE(std::isfinite(unit->pressure()));
        EXPECT_TRUE(std::isfinite(unit->humidity()));
        EXPECT_TRUE(std::isfinite(unit->gas()));

        EXPECT_FLOAT_EQ(unit->iaq(), unit->oldest().iaq());
        EXPECT_FLOAT_EQ(unit->temperature(), unit->oldest().temperature());
        EXPECT_FLOAT_EQ(unit->pressure(), unit->oldest().pressure());
        EXPECT_FLOAT_EQ(unit->humidity(), unit->oldest().humidity());
        EXPECT_FLOAT_EQ(unit->gas(), unit->oldest().gas());

        EXPECT_FALSE(unit->empty());
        unit->discard();
    }
#endif

    EXPECT_EQ(unit->available(), 4U);
    EXPECT_FALSE(unit->empty());
    EXPECT_FALSE(unit->full());

    unit->flush();
    EXPECT_EQ(unit->available(), 0U);
    EXPECT_TRUE(unit->empty());
    EXPECT_FALSE(unit->full());

    // Config
    EXPECT_EQ(sizeof(bsec_config), BSEC_MAX_PROPERTY_BLOB_SIZE);
    EXPECT_TRUE(unit->bsec2GetConfig(cfg, actual));  // get current
    EXPECT_NE(memcmp(cfg, bsec_config, actual), 0);

    EXPECT_TRUE(unit->bsec2SetConfig(bsec_config));  // overwrite
    EXPECT_TRUE(unit->bsec2GetConfig(cfg, actual));
    auto cmp = memcmp(cfg, bsec_config, actual) == 0;
    EXPECT_TRUE(cmp);
    if (!cmp) {
        M5_DUMPI(cfg, actual);
        M5_DUMPI(bsec_config, actual);
    }

    // State
    EXPECT_TRUE(unit->bsec2GetState(state2, actual));  // get current
    cmp = memcmp(state2, state, actual) == 0;
    EXPECT_FALSE(cmp);

    EXPECT_TRUE(unit->bsec2SetState(state));           // rewrite
    EXPECT_TRUE(unit->bsec2GetState(state2, actual));  // get
    cmp = memcmp(state2, state, actual) == 0;
    if (!cmp) {
        M5_DUMPI(state, actual);
        M5_DUMPI(state2, actual);
    }
}
#endif

TEST_P(TestBME688, SingleShot)
{
    SCOPED_TRACE(ustr);

    bme68xConf tph{};
    tph.os_temp = m5::stl::to_underlying(Oversampling::x2);
    tph.os_pres = m5::stl::to_underlying(Oversampling::x1);
    tph.os_hum  = m5::stl::to_underlying(Oversampling::x16);
    tph.filter  = m5::stl::to_underlying(Filter::None);
    tph.odr     = m5::stl::to_underlying(ODR::None);
    EXPECT_TRUE(unit->writeTPHSetting(tph));

    m5::unit::bme688::bme68xHeatrConf hs{};
    hs.enable     = true;
    hs.heatr_temp = 300;
    hs.heatr_dur  = 100;
    EXPECT_TRUE(unit->writeHeaterSetting(Mode::Forced, hs));

    bme68xData data{};

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_FALSE(unit->measureSingleShot(data));
    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    Mode m{};
    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Sleep);
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->measureSingleShot(data));

#if 0
    M5_LOGI(
        "Status:%u\n"
        "Gidx:%u Midx:%u\n"
        "ResHeate:%u IDAC:%u GasWait:%u\n"
        "T:%f P:%f H:%f R:%f",
        data.status, data.gas_index, data.meas_index, data.res_heat, data.idac, data.gas_wait, data.temperature,
        data.pressure, data.humidity, data.gas_resistance);
#endif
}

TEST_P(TestBME688, PeriodicForced)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    bme68xConf tph{};
    tph.os_temp = m5::stl::to_underlying(Oversampling::x2);
    tph.os_pres = m5::stl::to_underlying(Oversampling::x1);
    tph.os_hum  = m5::stl::to_underlying(Oversampling::x16);
    tph.filter  = m5::stl::to_underlying(Filter::None);
    tph.odr     = m5::stl::to_underlying(ODR::None);
    EXPECT_TRUE(unit->writeTPHSetting(tph));

    m5::unit::bme688::bme68xHeatrConf hs{};
    hs.enable     = true;
    hs.heatr_temp = 300;
    hs.heatr_dur  = 100;
    EXPECT_TRUE(unit->writeHeaterSetting(Mode::Forced, hs));

    //
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->startPeriodicMeasurement(Mode::Forced));
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_EQ(unit->mode(), Mode::Forced);
    // Always wait for an interval to obtain the correct value for the first measurement
    // EXPECT_EQ(unit->updatedMillis(), 0); //

    EXPECT_EQ(unit->available(), 0U);
    EXPECT_TRUE(unit->empty());
    EXPECT_FALSE(unit->full());

    test_periodic_measurement(unit.get(), 8, 8, (unit->interval() * 2) * 8, check_measurement_values, false);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_EQ(unit->mode(), Mode::Sleep);

    EXPECT_EQ(unit->available(), 8U);
    EXPECT_FALSE(unit->empty());
    EXPECT_TRUE(unit->full());

    uint32_t cnt{4};
    while (unit->available() && cnt--) {
        EXPECT_TRUE(std::isfinite(unit->oldest().raw_temperature()));
        EXPECT_TRUE(std::isfinite(unit->oldest().raw_pressure()));
        EXPECT_TRUE(std::isfinite(unit->oldest().raw_humidity()));
        EXPECT_TRUE(std::isfinite(unit->oldest().raw_gas()));

        EXPECT_FALSE(unit->empty());
        unit->discard();
    }

    EXPECT_EQ(unit->available(), 4U);
    EXPECT_FALSE(unit->empty());
    EXPECT_FALSE(unit->full());

    unit->flush();
    EXPECT_EQ(unit->available(), 0U);
    EXPECT_TRUE(unit->empty());
    EXPECT_FALSE(unit->full());
}

TEST_P(TestBME688, PeriodicParallel)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    uint16_t temp_prof[10] = {320, 100, 100, 100, 200, 200, 200, 320, 320, 320};
    /* Multiplier to the shared heater duration */
    uint16_t mul_prof[10] = {5, 2, 10, 30, 5, 5, 5, 5, 5, 5};

    bme68xConf tph{};
    tph.os_temp = m5::stl::to_underlying(Oversampling::x2);
    tph.os_pres = m5::stl::to_underlying(Oversampling::x1);
    tph.os_hum  = m5::stl::to_underlying(Oversampling::x16);
    tph.filter  = m5::stl::to_underlying(Filter::None);
    tph.odr     = m5::stl::to_underlying(ODR::None);
    EXPECT_TRUE(unit->writeTPHSetting(tph));

    m5::unit::bme688::bme68xHeatrConf hs{};
    hs.enable = true;
    memcpy(hs.temp_prof, temp_prof, sizeof(temp_prof));
    memcpy(hs.dur_prof, mul_prof, sizeof(mul_prof));
    hs.shared_heatr_dur = (uint16_t)(140 - (unit->calculateMeasurementInterval(Mode::Parallel, tph) / 1000));
    hs.profile_len      = 10;
    EXPECT_TRUE(unit->writeHeaterSetting(Mode::Parallel, hs));

    //
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->startPeriodicMeasurement(Mode::Parallel));
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_EQ(unit->mode(), Mode::Parallel);

    EXPECT_EQ(unit->available(), 0U);
    EXPECT_TRUE(unit->empty());
    EXPECT_FALSE(unit->full());

    // TODO : What are the measurement intervals in the parallel mode datasheet?
    test_periodic_measurement(unit.get(), 8, 1, (unit->interval() * 10) * 10, check_measurement_values, false);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_EQ(unit->mode(), Mode::Sleep);

    EXPECT_EQ(unit->available(), 8U);
    EXPECT_FALSE(unit->empty());
    EXPECT_TRUE(unit->full());

    uint32_t cnt{4};
    while (unit->available() && cnt--) {
        EXPECT_TRUE(std::isfinite(unit->oldest().raw_temperature()));
        EXPECT_TRUE(std::isfinite(unit->oldest().raw_pressure()));
        EXPECT_TRUE(std::isfinite(unit->oldest().raw_humidity()));
        EXPECT_TRUE(std::isfinite(unit->oldest().raw_gas()));

        EXPECT_FALSE(unit->empty());
        unit->discard();
    }

    EXPECT_EQ(unit->available(), 4U);
    EXPECT_FALSE(unit->empty());
    EXPECT_FALSE(unit->full());

    unit->flush();
    EXPECT_EQ(unit->available(), 0U);
    EXPECT_TRUE(unit->empty());
    EXPECT_FALSE(unit->full());
}

TEST_P(TestBME688, PeriodiSequential)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    uint16_t temp_prof[10] = {200, 240, 280, 320, 360, 360, 320, 280, 240, 200};
    /* Heating duration in milliseconds */
    uint16_t dur_prof[10] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};

    bme68xConf tph{};
    tph.os_temp = m5::stl::to_underlying(Oversampling::x2);
    tph.os_pres = m5::stl::to_underlying(Oversampling::x1);
    tph.os_hum  = m5::stl::to_underlying(Oversampling::x16);
    tph.filter  = m5::stl::to_underlying(Filter::None);
    tph.odr     = m5::stl::to_underlying(ODR::None);
    EXPECT_TRUE(unit->writeTPHSetting(tph));

    m5::unit::bme688::bme68xHeatrConf hs{};
    hs.enable = true;
    memcpy(hs.temp_prof, temp_prof, sizeof(temp_prof));
    memcpy(hs.dur_prof, dur_prof, sizeof(dur_prof));
    hs.profile_len = 10;
    EXPECT_TRUE(unit->writeHeaterSetting(Mode::Sequential, hs));

    //
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->startPeriodicMeasurement(Mode::Sequential));
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_EQ(unit->mode(), Mode::Sequential);

    test_periodic_measurement(unit.get(), 8, 1, (unit->interval() * 2) * 8, check_measurement_values, false);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_EQ(unit->mode(), Mode::Sleep);
}

TEST_P(TestBME688, SelfTest)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->selfTest());
}
