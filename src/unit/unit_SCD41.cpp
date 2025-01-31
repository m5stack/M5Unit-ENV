/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SCD41.cpp
  @brief SCD41 Unit for M5UnitUnified
*/
#include "unit_SCD41.hpp"
#include <M5Utility.hpp>
#include <array>
using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::scd4x;
using namespace m5::unit::scd4x::command;
using namespace m5::unit::scd41;
using namespace m5::unit::scd41::command;

namespace {
// Max command duration(ms)
constexpr uint16_t MEASURE_SINGLE_SHOT_DURATION{5000};
constexpr uint16_t MEASURE_SINGLE_SHOT_RHT_ONLY_DURATION{50};
}  // namespace

namespace m5 {
namespace unit {
// class UnitSCD41
const char UnitSCD41::name[] = "UnitSCD41";
const types::uid_t UnitSCD41::uid{"UnitSCD41"_mmh3};
const types::uid_t UnitSCD41::attr{0};

bool UnitSCD41::measureSingleshot(Data& d)
{
    d = Data{};

    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    if (writeRegister(MEASURE_SINGLE_SHOT)) {
        m5::utility::delay(MEASURE_SINGLE_SHOT_DURATION);
        return read_measurement(d);
    }
    return false;
}

bool UnitSCD41::measureSingleshotRHT(Data& d)
{
    d = Data{};

    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    if (writeRegister(MEASURE_SINGLE_SHOT_RHT_ONLY)) {
        m5::utility::delay(MEASURE_SINGLE_SHOT_RHT_ONLY_DURATION);
        return read_measurement(d, false);
    }
    return false;
}

}  // namespace unit
}  // namespace m5
