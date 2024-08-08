/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ENV3.cpp
  @brief ENV III Unit for M5UnitUnified
*/
#include "unit_ENV3.hpp"
#include <M5Utility.hpp>

namespace m5 {
namespace unit {

using namespace m5::utility::mmh3;

const char UnitENV3::name[] = "UnitENVIII";
const types::uid_t UnitENV3::uid{"UnitENVIII"_mmh3};
const types::uid_t UnitENV3::attr{0};

UnitENV3::UnitENV3(const uint8_t addr) : Component(addr) {
    // Form a parent-child relationship
    auto cfg         = component_config();
    cfg.max_children = 2;
    component_config(cfg);
    _valid = add(sht30, 0) && add(qmp6988, 1);
}

Adapter* UnitENV3::ensure_adapter(const uint8_t ch) {
    if (ch >= _adapters.size()) {
        M5_LIB_LOGE("Invalid channel %u", ch);
        return nullptr;
    }
    auto unit = child(ch);
    if (!unit) {
        M5_LIB_LOGE("Not exists unit %u", ch);
        return nullptr;
    }
    auto& ad = _adapters[ch];
    if (!ad) {
        ad.reset(_adapter->duplicate(unit->address()));
        // ad.reset(new PaHubAdapter(unit->address()));
    }
    return ad.get();
}

}  // namespace unit
}  // namespace m5
