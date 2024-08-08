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
    explicit UnitSCD41(const uint8_t addr = DEFAULT_ADDRESS) : UnitSCD40(addr) {
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

}  // namespace unit
}  // namespace m5
#endif
