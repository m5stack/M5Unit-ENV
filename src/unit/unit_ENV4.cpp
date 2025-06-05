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
using namespace m5::unit::types;

const char UnitENV4::name[] = "UnitENV4";
const types::uid_t UnitENV4::uid{"UnitENV4"_mmh3};
const types::attr_t UnitENV4::attr{attribute::AccessI2C};

UnitENV4::UnitENV4(const uint8_t addr) : Component(addr)
{
    // Form a parent-child relationship
    auto cfg         = component_config();
    cfg.max_children = 2;
    component_config(cfg);
    _valid = add(sht40, 0) && add(bmp280, 1);
}

std::shared_ptr<Adapter> UnitENV4::ensure_adapter(const uint8_t ch)
{
    if (ch > 2) {
        M5_LIB_LOGE("Invalid channel %u", ch);
        return std::make_shared<Adapter>();  // Empty adapter
    }
    auto unit = child(ch);
    if (!unit) {
        M5_LIB_LOGE("Not exists unit %u", ch);
        return std::make_shared<Adapter>();  // Empty adapter
    }
    auto ad = asAdapter<AdapterI2C>(Adapter::Type::I2C);
    return ad ? std::shared_ptr<Adapter>(ad->duplicate(unit->address())) : std::make_shared<Adapter>();
}

}  // namespace unit
}  // namespace m5
