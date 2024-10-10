/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ENV3.hpp
  @brief ENV III Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ENV_UNIT_ENV3_HPP
#define M5_UNIT_ENV_UNIT_ENV3_HPP

#include <M5UnitComponent.hpp>
#include <array>
#include "unit_SHT30.hpp"
#include "unit_QMP6988.hpp"

namespace m5 {
namespace unit {

/*!
  @class UnitENV3
  @brief ENV III is an environmental sensor that integrates SHT30 and QMP6988
  @details This unit itself has no I/O, but holds SHT30 and QMP6988 instance
 */
class UnitENV3 : public Component {
    // Must not be 0x00 for ensure and assign adapter to children
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitENV3, 0x44 /* Dummy address */);

public:
    UnitSHT30 sht30;      //!< @brief SHT30 instance
    UnitQMP6988 qmp6988;  //!< @brief QMP6988 instance

    explicit UnitENV3(const uint8_t addr = DEFAULT_ADDRESS);
    virtual ~UnitENV3()
    {
    }

    virtual bool begin() override
    {
        return _valid;
    }

protected:
    virtual Adapter* duplicate_adapter(const uint8_t ch) override;

private:
    bool _valid{};  // Did the constructor correctly add the child unit?
    Component* _children[2]{&sht30, &qmp6988};
};

}  // namespace unit
}  // namespace m5
#endif
