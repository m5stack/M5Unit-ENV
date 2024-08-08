/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file scd4x.hpp
  @brief Definitions for SCD4x families
*/
#ifndef M5_UNIT_ENV_UNIT_INTERNAL_SCD4x_HPP
#define M5_UNIT_ENV_UNIT_INTERNAL_SCD4x_HPP

#include <cstdint>
#include <array>

namespace m5 {
namespace unit {
/*!
  @namespace scd4x
  @brief For SCD40/41
 */
namespace scd4x {
/*!
  @enum Mode
  @brief Mode of periodic measurement
 */
enum class Mode : uint8_t {
    Normal,    //!< Normal (Receive data every 5 seconds)
    LowPower,  //!< Low power (Receive data every 30 seconds)
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 9> raw{};  //!< RAW data
    uint16_t co2() const;          //!< CO2 concentration (ppm)
    //! temperature (Celsius)
    inline float temperature() const {
        return celsius();
    }
    float celsius() const;     //!< temperature (Celsius)
    float fahrenheit() const;  //!< temperature (Fahrenheit)
    float humidity() const;        //!< humidity (RH)
};

///@cond
// Max command duration(ms)
// For SCD40/41
constexpr uint16_t READ_MEASUREMENT_DURATION{1};
constexpr uint16_t STOP_PERIODIC_MEASUREMENT_DURATION{500};
constexpr uint16_t SET_TEMPERATURE_OFFSET_DURATION{1};
constexpr uint16_t GET_TEMPERATURE_OFFSET_DURATION{1};
constexpr uint16_t SET_SENSOR_ALTITUDE_DURATION{1};
constexpr uint16_t GET_SENSOR_ALTITUDE_DURATION{1};
constexpr uint16_t SET_AMBIENT_PRESSURE_DURATION{1};
constexpr uint16_t PERFORM_FORCED_CALIBRATION_DURATION{400};
constexpr uint16_t SET_AUTOMATIC_SELF_CALIBRATION_ENABLED_DURATION{1};
constexpr uint16_t GET_AUTOMATIC_SELF_CALIBRATION_ENABLED_DURATION{1};
constexpr uint16_t GET_DATA_READY_STATUS_DURATION{1};
constexpr uint16_t PERSIST_SETTINGS_DURATION{800};
constexpr uint16_t GET_SERIAL_NUMBER_DURATION{1};
constexpr uint16_t PERFORM_SELF_TEST_DURATION{10000};
constexpr uint16_t PERFORM_FACTORY_RESET_DURATION{1200};
constexpr uint16_t REINIT_DURATION{20};

namespace command {
// Basic Commands
constexpr uint16_t START_PERIODIC_MEASUREMENT{0x21b1};
constexpr uint16_t READ_MEASUREMENT{0xec05};
constexpr uint16_t STOP_PERIODIC_MEASUREMENT{0x3f86};
// On-chip output signal compensation
constexpr uint16_t SET_TEMPERATURE_OFFSET{0x241d};
constexpr uint16_t GET_TEMPERATURE_OFFSET{0x2318};
constexpr uint16_t SET_SENSOR_ALTITUDE{0x2427};
constexpr uint16_t GET_SENSOR_ALTITUDE{0x2322};
constexpr uint16_t SET_AMBIENT_PRESSURE{0xe000};
// Field calibration
constexpr uint16_t PERFORM_FORCED_CALIBRATION{0x362f};
constexpr uint16_t SET_AUTOMATIC_SELF_CALIBRATION_ENABLED{0x2416};
constexpr uint16_t GET_AUTOMATIC_SELF_CALIBRATION_ENABLED{0x2313};
// Low power
constexpr uint16_t START_LOW_POWER_PERIODIC_MEASUREMENT{0x21ac};
constexpr uint16_t GET_DATA_READY_STATUS{0xe4b8};
// Advanced features
constexpr uint16_t PERSIST_SETTINGS{0x3615};
constexpr uint16_t GET_SERIAL_NUMBER{0x3682};
constexpr uint16_t PERFORM_SELF_TEST{0x3639};
constexpr uint16_t PERFORM_FACTORY_RESET{0x3632};
constexpr uint16_t REINIT{0x3646};
}  // namespace command
///@endcond
}  // namespace scd4x

/*!
  @namespace scd41
  @brief For SCD41
 */
namespace scd41 {
///@cond
// Max command duration(ms)
constexpr uint16_t MEASURE_SINGLE_SHOT_DURATION{5000};
constexpr uint16_t MEASURE_SINGLE_SHOT_RHT_ONLY_DURATION{50};

namespace command {
// Low power single shot - SCD41 only
constexpr uint16_t MEASURE_SINGLE_SHOT{0x219d};
constexpr uint16_t MEASURE_SINGLE_SHOT_RHT_ONLY{0x2196};
}  // namespace command
///@endcond
}  // namespace scd41

}  // namespace unit
}  // namespace m5
#endif
