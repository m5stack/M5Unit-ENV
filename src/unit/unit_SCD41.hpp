/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SCD41.hpp
  @brief SCD41 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ENV_UNIT_SCD41_HPP
#define M5_UNIT_ENV_UNIT_SCD41_HPP

#include "unit_SCD40.hpp"

namespace m5 {
namespace unit {
/*!
  @namespace scd41
  @brief For SCD41
 */
namespace scd41 {
///@cond
// Max command duration(ms)
// For SCD40/41
constexpr uint32_t POWER_DOWN_DURATION{1};
constexpr uint32_t GET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD_DURATION{1};
constexpr uint32_t SET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD_DURATION{1};
constexpr uint32_t GET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD_DURATION{1};
constexpr uint32_t SET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD_DURATION{1};

///@endcond
}  // namespace scd41

/*!
  @class m5::unit::UnitSCD41
  @brief SCD41 unit component
*/
class UnitSCD41 : public UnitSCD40 {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSCD41, 0x62);

public:
    explicit UnitSCD41(const uint8_t addr = DEFAULT_ADDRESS) : UnitSCD40(addr)
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitSCD41()
    {
    }

    ///@name Single Shot Measurement Mode
    ///@{
    /*!
      @brief Request a single measurement
      @param[out] d Measurement data
      @return True if successful
      @note Blocked until measurement results are acquired (5000 ms)
      @warning During periodic detection runs, an error is returned
    */
    bool measureSingleshot(scd4x::Data& d);
    /*!
      @brief Request a single measurement temperature and humidity
      @param[out] d Measurement data
      @return True if successful
      @note Values are updated at 50 ms interval
      @note Blocked until measurement results are acquired (50 ms)
      @warning Information on CO2 is invalid.
      @warning During periodic detection runs, an error is returned
    */
    bool measureSingleshotRHT(scd4x::Data& d);
    ///@}

    ///@name Power mode
    ///@{
    /*!
      @brief Power down
      @details The sensor into sleep mode
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool powerDown(const uint32_t duration = scd41::POWER_DOWN_DURATION);
    /*!
      @brief Wake up
      @details The sensor from sleep mode into idle mode
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool wakeup();
    ///@}

    ///@name For ASC(Auto Self-Calibration)
    ///@{
    /*!
      @brief Write the duration of the initial period for ASC correction
      @param hours ASC initial period
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Allowed values are integer multiples of 4 hours
     */
    bool writeAutomaticSelfCalibrationInitialPeriod(
        const uint16_t hours, const uint32_t duration = scd41::SET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD_DURATION);
    /*!
      @brief Read the duration of the initial period for ASC correction
      @param[out] hours ASC initial period
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool readAutomaticSelfCalibrationInitialPeriod(uint16_t& hours);
    /*!
      @brief Write the standard period for ASC correction
      @param hours ASC standard period
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Allowed values are integer multiples of 4 hours
     */
    bool writeAutomaticSelfCalibrationStandardPeriod(
        const uint16_t hours, const uint32_t duration = scd41::SET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD_DURATION);
    /*!
      @brief Red the standard period for ASC correction
      @param[iut] hours ASC standard period
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Allowed values are integer multiples of 4 hours
     */
    bool readAutomaticSelfCalibrationStandardPeriod(uint16_t& hours);
    ///@}

protected:
    virtual bool is_valid_chip() override;
};

namespace scd41 {
///@cond
namespace command {
// Low power single shot - SCD41 only
constexpr uint16_t MEASURE_SINGLE_SHOT{0x219d};
constexpr uint16_t MEASURE_SINGLE_SHOT_RHT_ONLY{0x2196};
constexpr uint16_t POWER_DOWN{0x36e0};
constexpr uint16_t WAKE_UP{0x36f6};
constexpr uint16_t SET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD{0x2445};
constexpr uint16_t GET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD{0x2340};
constexpr uint16_t SET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD{0x244e};
constexpr uint16_t GET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD{0x234b};

}  // namespace command
///@endcond
}  // namespace scd41

}  // namespace unit
}  // namespace m5
#endif
