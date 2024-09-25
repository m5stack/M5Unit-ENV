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
  @class UnitSCD41
  @brief SCD41 unit component
*/
class UnitSCD41 : public UnitSCD40 {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSCD41, 0x00);

public:
    explicit UnitSCD41(const uint8_t addr = DEFAULT_ADDRESS) : UnitSCD40(addr)
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }

    ///@name Low power single shot (SCD41)
    ///@{
    /*!
      @brief Request a single measurement
      @return True if successful
      @note Values are updated at 5000 ms interval
      @warning During periodic detection runs, an error is returned
    */
    bool measureSingleshot(scd4x::Data &d);
    /*!
      @brief Request a single measurement temperature and humidity
      @return True if successful
      @note Values are updated at 50 ms interval
      @warning Information on CO2 is invalid.
      @warning During periodic detection runs, an error is returned
    */
    bool measureSingleshotRHT(scd4x::Data &d);
    ///@}
};

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
