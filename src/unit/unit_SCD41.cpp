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

const uint8_t VARIANT_VALUE[2]{0x14, 0x40};  // SCD41

}  // namespace

namespace m5 {
namespace unit {
// class UnitSCD41
const char UnitSCD41::name[] = "UnitSCD41";
const types::uid_t UnitSCD41::uid{"UnitSCD41"_mmh3};
const types::attr_t UnitSCD41::attr{};

bool UnitSCD41::is_valid_chip()
{
    uint8_t var[2]{};
    if (!read_register(GET_SENSOR_VARIANT, var, 2) || memcmp(var, VARIANT_VALUE, 2) != 0) {
        M5_LIB_LOGE("Not SCD40 %02X:%02X", var[0], var[1]);
        return false;
    }
    return true;
}

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

bool UnitSCD41::powerDown(const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (writeRegister(POWER_DOWN, nullptr, 0)) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitSCD41::wakeup(const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    // Note that the SCD4x does not acknowledge the wake_up command
    writeRegister(WAKE_UP, nullptr, 0);
    m5::utility::delay(duration);

    // The sensor’s idle state after wake up can be verified by reading out the serial numbe
    uint64_t sn{};
    return readSerialNumber(sn);
}

bool UnitSCD41::writeAutomaticSelfCalibrationInitialPeriod(const uint16_t hours, const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (hours % 4) {
        M5_LIB_LOGW("Arguments are modified to multiples of 4");
    }
    uint16_t h = (hours >> 2) << 2;
    m5::types::big_uint16_t u16{h};
    return write_register(SET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD, u16.data(), u16.size()) &&
           delay_true(duration);
}

bool UnitSCD41::readAutomaticSelfCalibrationInitialPeriod(uint16_t& hours)
{
    hours = 0;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    m5::types::big_uint16_t u16{};
    if (read_register(GET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD, u16.data(), u16.size(),
                      GET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD_DURATION)) {
        hours = u16.get();
        return true;
    }
    return false;
}

bool UnitSCD41::writeAutomaticSelfCalibrationStandardPeriod(const uint16_t hours, const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (hours % 4) {
        M5_LIB_LOGW("Arguments are modified to multiples of 4");
    }
    uint16_t h = (hours >> 2) << 2;
    m5::types::big_uint16_t u16{h};
    return write_register(SET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD, u16.data(), u16.size()) &&
           delay_true(duration);
}

bool UnitSCD41::readAutomaticSelfCalibrationStandardPeriod(uint16_t& hours)
{
    hours = 0;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    m5::types::big_uint16_t u16{};
    if (read_register(GET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD, u16.data(), u16.size(),
                      GET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD_DURATION)) {
        hours = u16.get();
        return true;
    }
    return false;
}

}  // namespace unit
}  // namespace m5
