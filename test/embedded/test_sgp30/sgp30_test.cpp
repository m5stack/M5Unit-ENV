/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitSGP30
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_SGP30.hpp>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::sgp30;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestSGP30 : public ComponentTestBase<UnitSGP30, bool> {
protected:
    virtual UnitSGP30* get_instance() override
    {
        auto* ptr = new m5::unit::UnitSGP30();
        if (ptr) {
            auto ccfg        = ptr->component_config();
            ccfg.stored_size = 4;
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

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSGP30, ::testing::Values(false, true));
//   INSTANTIATE_TEST_SUITE_P(ParamValues, TestSGP30, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSGP30, ::testing::Values(false));

namespace {
void check_measurement_values(UnitSGP30* u)
{
    EXPECT_NE(u->co2eq(), 0xFFFFU);
    EXPECT_NE(u->tvoc(), 0XFFFFU);

    // readRaw test
    uint16_t h{}, e{};
    EXPECT_TRUE(u->readRaw(h, e));
    EXPECT_NE(h, 0U);
    EXPECT_NE(e, 0U);
}

void wait15sec()
{
    auto to = m5::utility::millis() + 15 * 1000;
    do {
        m5::utility::delay(1);
    } while (m5::utility::millis() < to);
}

}  // namespace

TEST_P(TestSGP30, FeatureSet)
{
    SCOPED_TRACE(ustr);

    EXPECT_NE(unit->productVersion(), 0U);
    M5_LOGI("productVersion:%x", unit->productVersion());

    Feature f{};
    EXPECT_TRUE(unit->readFeatureSet(f));
    EXPECT_EQ(f.productType(), 0U);
    EXPECT_NE(f.productVersion(), 0U);

    EXPECT_EQ(unit->productVersion(), f.productVersion());
}

TEST_P(TestSGP30, selfTest)
{
    SCOPED_TRACE(ustr);

    uint16_t result{};
    EXPECT_TRUE(unit->measureTest(result));
    EXPECT_EQ(result, 0xD400);
}

TEST_P(TestSGP30, serialNumber)
{
    SCOPED_TRACE(ustr);
    // Read direct [MSB] SNB_3, SNB_2, CRC, SNB_1, SNB_0, CRC [LSB]
    std::array<uint8_t, 9> rbuf{};
    EXPECT_TRUE(unit->readRegister(command::GET_SERIAL_ID, rbuf.data(), rbuf.size(), 1));

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

TEST_P(TestSGP30, generalReset)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->startPeriodicMeasurement(0x1234, 0x5678, 0x9ABC));
    M5_LOGW("SGP30 measurement starts 15 seconds after begin");
    EXPECT_TRUE(unit->inPeriodic());
    wait15sec();

    uint16_t co2eq{}, tvoc{}, inceptive_tvoc{};
    EXPECT_TRUE(unit->readIaqBaseline(co2eq, tvoc));
    // EXPECT_TRUE(unit->readTvocInceptiveBaseline(inceptive_tvoc));

    // M5_LOGW("%x/%x/%x", co2eq, tvoc, inceptive_tvoc);

    EXPECT_EQ(co2eq, 0x1234);
    EXPECT_EQ(tvoc, 0x5678);
    // EXPECT_EQ(inceptive_tvoc, 0x1122);

    EXPECT_TRUE(unit->generalReset());

    EXPECT_TRUE(unit->readIaqBaseline(co2eq, tvoc));
    // EXPECT_TRUE(unit->readTvocInceptiveBaseline(inceptive_tvoc));
    // M5_LOGW("%x/%x/%x", co2eq, tvoc, inceptive_tvoc);

    EXPECT_EQ(co2eq, 0x0000);
    EXPECT_EQ(tvoc, 0x0000);
    // EXPECT_EQ(inceptive_tvoc, 0x0000);
}

TEST_P(TestSGP30, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_FALSE(unit->inPeriodic());

    {
        uint32_t cnt{10};
        uint16_t h2{}, et{};
        while (cnt--) {
            EXPECT_TRUE(unit->readRaw(h2, et));
        }
    }

    EXPECT_TRUE(unit->startPeriodicMeasurement(0, 0, 0));
    M5_LOGW("SGP30 measurement starts 15 seconds after begin");
    EXPECT_TRUE(unit->inPeriodic());
    wait15sec();

    test_periodic_measurement(unit.get(), 4, check_measurement_values);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_EQ(unit->available(), 4);
    EXPECT_TRUE(unit->full());
    EXPECT_FALSE(unit->empty());

    uint32_t cnt{2};
    while (unit->available() && cnt--) {
        EXPECT_FALSE(unit->empty());
        // M5_LOGW("C:%u T:%u", unit->co2eq(), unit->tvoc());
        EXPECT_EQ(unit->co2eq(), unit->oldest().co2eq());
        EXPECT_EQ(unit->tvoc(), unit->oldest().tvoc());
        unit->discard();
    }

    EXPECT_EQ(unit->available(), 2);
    EXPECT_FALSE(unit->full());
    EXPECT_FALSE(unit->empty());

    unit->flush();
    EXPECT_EQ(unit->co2eq(), 0XFFFF);
    EXPECT_EQ(unit->tvoc(), 0XFFFF);
    EXPECT_EQ(unit->available(), 0);
    EXPECT_TRUE(unit->empty());
    EXPECT_FALSE(unit->full());
}
