/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for Unit/HatENVIII
  Required
  - M5Unified: https://github.com/m5stack/M5Unified
*/
// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_ENV3) && !defined(USING_HAT_ENV3)
#define USING_UNIT_ENV3  // Default: UnitENV3 (GROVE port)
// #define USING_HAT_ENV3
#endif
// *************************************************************
#include "main/PlotToSerial.cpp"
