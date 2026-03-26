/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_HatYun.cpp
  @brief Hat Yun for M5UnitUnified
*/
#include "unit_HatYun.hpp"
#include <M5Utility.hpp>

namespace m5 {
namespace unit {

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::hatyun;
using namespace m5::unit::hatyun::command;

const char HatYun::name[] = "HatYun";
const types::uid_t HatYun::uid{"HatYun"_mmh3};
const types::attr_t HatYun::attr{attribute::AccessI2C};

HatYun::HatYun(const uint8_t addr) : Component(addr), _data{new m5::container::CircularBuffer<hatyun::Data>(1)}
{
    auto cfg         = component_config();
    cfg.max_children = 2;
    component_config(cfg);
    _valid = add(sht20, 0) && add(bmp280, 1);
}

bool HatYun::begin()
{
    if (!_valid) {
        return false;
    }

    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<hatyun::Data>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    // Verify STM32 communication by reading light sensor
    uint16_t val{};
    if (!readLight(val)) {
        M5_LIB_LOGE("Failed to read light sensor");
        return false;
    }

    // Turn off all LEDs on init
    writeAllLED(0, 0, 0);

    return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.periodic_interval) : true;
}

void HatYun::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            hatyun::Data d{};
            _updated = read_measurement(d);
            if (_updated) {
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool HatYun::start_periodic_measurement(const uint32_t interval)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    _interval = interval < 10 ? 10 : interval;
    _latest   = 0;
    _periodic = true;
    return true;
}

bool HatYun::stop_periodic_measurement()
{
    _periodic = false;
    _latest   = 0;
    return true;
}

bool HatYun::read_measurement(hatyun::Data& d)
{
    return readLight(d.light);
}

bool HatYun::writeLED(const uint8_t num, const uint8_t r, const uint8_t g, const uint8_t b)
{
    if (num >= NUM_LEDS) {
        return false;
    }
    uint8_t buf[5] = {LED_REG, num, r, g, b};
    return writeWithTransaction(buf, sizeof(buf)) == m5::hal::error::error_t::OK;
}

bool HatYun::writeAllLED(const uint8_t r, const uint8_t g, const uint8_t b)
{
    bool ok = true;
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        if (!writeLED(i, r, g, b)) {
            ok = false;
        }
    }
    return ok;
}

bool HatYun::writeRainbow(const uint8_t offset, const uint8_t brightness)
{
    bool ok = true;
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        uint8_t hue    = offset + (i * 256 / NUM_LEDS);  // Intentional uint8_t wrap-around for hue cycling
        uint8_t region = hue / 43;
        uint8_t frac   = (hue - region * 43) * 6;
        uint8_t q      = (brightness * (255 - frac)) >> 8;
        uint8_t t      = (brightness * frac) >> 8;
        uint8_t r, g, b;
        switch (region) {
            case 0:
                r = brightness;
                g = t;
                b = 0;
                break;
            case 1:
                r = q;
                g = brightness;
                b = 0;
                break;
            case 2:
                r = 0;
                g = brightness;
                b = t;
                break;
            case 3:
                r = 0;
                g = q;
                b = brightness;
                break;
            case 4:
                r = t;
                g = 0;
                b = brightness;
                break;
            default:
                r = brightness;
                g = 0;
                b = q;
                break;
        }
        if (!writeLED(i, r, g, b)) {
            ok = false;
        }
    }
    return ok;
}

bool HatYun::readLight(uint16_t& value)
{
    uint8_t cmd = LIGHT_REG;
    if (writeWithTransaction(&cmd, 1) != m5::hal::error::error_t::OK) {
        return false;
    }
    uint8_t buf[2]{};
    if (readWithTransaction(buf, 2) != m5::hal::error::error_t::OK) {
        return false;
    }
    value = (static_cast<uint16_t>(buf[1]) << 8) | buf[0];
    return true;
}

std::shared_ptr<Adapter> HatYun::ensure_adapter(const uint8_t ch)
{
    if (ch >= 2) {
        M5_LIB_LOGE("Invalid channel %u", ch);
        return std::make_shared<Adapter>();
    }
    auto unit = child(ch);
    if (!unit) {
        M5_LIB_LOGE("Not exists unit %u", ch);
        return std::make_shared<Adapter>();
    }
    auto ad = asAdapter<AdapterI2C>(Adapter::Type::I2C);
    return ad ? std::shared_ptr<Adapter>(ad->duplicate(unit->address())) : std::make_shared<Adapter>();
}

}  // namespace unit
}  // namespace m5
