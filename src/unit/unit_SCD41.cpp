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

namespace m5 {
namespace unit {
// class UnitSCD41
const char UnitSCD41::name[] = "UnitSCD41";
const types::uid_t UnitSCD41::uid{"UnitSCD41"_mmh3};
const types::uid_t UnitSCD41::attr{0};

bool UnitSCD41::measureSingleshot(Data& d)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    return writeRegister(MEASURE_SINGLE_SHOT) && read_measurement(d);
}

bool UnitSCD41::measureSingleshotRHT(Data& d)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    return writeRegister(MEASURE_SINGLE_SHOT_RHT_ONLY) && read_measurement(d, false);
}

}  // namespace unit
}  // namespace m5
