/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ENV4.hpp
  @brief ENV IV Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ENV_UNIT_ENV4_HPP
#define M5_UNIT_ENV_UNIT_ENV4_HPP

#include <M5UnitComponent.hpp>
#include <array>
#include "unit_SHT40.hpp"
#include "unit_BMP280.hpp"

namespace m5 {
namespace unit {

/*!
  @class UnitENV4
  @brief ENV IV is an environmental sensor that integrates SHT40 and BMP280
  @details This unit itself has no I/O, but holds SHT40 and BMP280 instance
 */
class UnitENV4 : public Component {
    // Must not be 0x00 for ensure and assign adapter to children
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitENV4, 0xFF /* Dummy address */);

public:
    UnitSHT40 sht40;    //!< @brief SHT40 instance
    UnitBMP280 bmp280;  //!< @brief BMP280 instance

    explicit UnitENV4(const uint8_t addr = DEFAULT_ADDRESS);
    virtual ~UnitENV4()
    {
    }

    virtual bool begin() override
    {
        return _valid;
    }

protected:
    virtual std::shared_ptr<Adapter> ensure_adapter(const uint8_t ch);

private:
    bool _valid{};  // Did the constructor correctly add the child unit?
    Component* _children[2]{&sht40, &bmp280};
};

}  // namespace unit
}  // namespace m5
#endif
