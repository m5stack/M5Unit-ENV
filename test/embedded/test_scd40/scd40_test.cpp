/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitSCD40
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_SCD40.hpp>
#include <chrono>
#include <iostream>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::scd4x;
using m5::unit::types::elapsed_time_t;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

constexpr uint32_t STORED_SIZE{4};

class TestSCD4x : public ComponentTestBase<UnitSCD40, bool> {
protected:
    virtual UnitSCD40* get_instance() override
    {
        auto ptr         = new m5::unit::UnitSCD40();
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

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD4x,  ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD4x, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSCD4x, ::testing::Values(false));

namespace {
}  // namespace

#include "../scd4x_test.inl"
