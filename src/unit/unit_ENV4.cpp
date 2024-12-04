/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ENV4.cpp
  @brief ENV 4 Unit for M5UnitUnified
*/
#include "unit_ENV4.hpp"
#include <M5Utility.hpp>

namespace m5 {
namespace unit {

using namespace m5::utility::mmh3;

const char UnitENV4::name[] = "UnitENVIV";
const types::uid_t UnitENV4::uid{"UnitENVIV"_mmh3};
const types::uid_t UnitENV4::attr{0};

UnitENV4::UnitENV4(const uint8_t addr) : Component(addr)
{
    // Form a parent-child relationship
    auto cfg         = component_config();
    cfg.max_children = 2;
    component_config(cfg);
    _valid = add(sht40, 0) && add(bmp280, 1);
}

Adapter* UnitENV4::duplicate_adapter(const uint8_t ch)
{
    if (ch >= 2) {
        M5_LIB_LOGE("Invalid channel %u", ch);
        return nullptr;
    }
    auto unit = _children[ch];
    return adapter()->duplicate(unit->address());
}

}  // namespace unit
}  // namespace m5
