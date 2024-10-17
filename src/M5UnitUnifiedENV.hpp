/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file M5UnitUnifiedENV.hpp
  @brief Main header of M5UnitENV using M5UnitUnified

  @mainpage M5Unit-ENV
  Library for UnitENV using M5UnitUnified.
*/
#if defined(_M5_UNIT_ENV_H_)
#error "DO NOT USE it at the same time as conventional libraries"
#endif

#ifndef M5_UNIT_UNIFIED_ENV_HPP
#define M5_UNIT_UNIFIED_ENV_HPP

#include "unit/unit_SCD40.hpp"
#include "unit/unit_SCD41.hpp"

#include "unit/unit_SHT30.hpp"
#include "unit/unit_QMP6988.hpp"
#include "unit/unit_ENV3.hpp"

#include "unit/unit_BME688.hpp"

#include "unit/unit_SGP30.hpp"

/*!
  @namespace m5
  @brief Top level namespace of M5stack
 */
namespace m5 {

/*!
  @namespace unit
  @brief Unit-related namespace
 */
namespace unit {

using UnitCO2    = m5::unit::UnitSCD40;
using UnitENVPro = m5::unit::UnitBME688;
using UnitTVOC   = m5::unit::UnitSGP30;

}  // namespace unit
}  // namespace m5
#endif
