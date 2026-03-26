/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SHT20.cpp
  @brief SHT20 Unit for M5UnitUnified
 */
#include "unit_SHT20.hpp"
#include <M5Utility.hpp>
#include <array>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::sht20;
using namespace m5::unit::sht20::command;

namespace {

// SHT20 CRC-8: polynomial 0x31, init 0x00 (differs from SHT30/SHT40 which use init 0xFF)
m5::utility::CRC8 crc_sht20(0x00, 0x31, false, false, 0x00);

// Measurement duration max (ms) by resolution — Table 7
// Index: 0=RH12_T14, 1=RH8_T12, 2=RH10_T13, 3=RH11_T11
constexpr elapsed_time_t temp_duration[] = {
    85,  // 14-bit
    22,  // 12-bit
    43,  // 13-bit
    11,  // 11-bit
};
constexpr elapsed_time_t humd_duration[] = {
    29,  // 12-bit
    4,   // 8-bit
    9,   // 10-bit
    15,  // 11-bit
};

uint8_t resolution_index(Resolution res)
{
    switch (res) {
        case Resolution::RH12_T14:
            return 0;
        case Resolution::RH8_T12:
            return 1;
        case Resolution::RH10_T13:
            return 2;
        case Resolution::RH11_T11:
            return 3;
        default:
            return 0;
    }
}

}  // namespace

namespace m5 {
namespace unit {
namespace sht20 {

float Data::celsius() const
{
    // Temperature: -46.85 + 175.72 * S_T / 2^16
    // raw[0..2] = MSB, LSB, CRC  (status bits in LSB[1:0] must be masked)
    uint16_t raw_val = (static_cast<uint16_t>(raw[0]) << 8) | (raw[1] & 0xFC);
    return -46.85f + 175.72f * raw_val / 65536.f;
}

float Data::fahrenheit() const
{
    return celsius() * 9.0f / 5.0f + 32.f;
}

float Data::humidity() const
{
    // Humidity: -6 + 125 * S_RH / 2^16
    // raw[3..5] = MSB, LSB, CRC  (status bits in LSB[1:0] must be masked)
    uint16_t raw_val = (static_cast<uint16_t>(raw[3]) << 8) | (raw[4] & 0xFC);
    return -6.f + 125.f * raw_val / 65536.f;
}

}  // namespace sht20

const char UnitSHT20::name[] = "UnitSHT20";
const types::uid_t UnitSHT20::uid{"UnitSHT20"_mmh3};
const types::attr_t UnitSHT20::attr{attribute::AccessI2C};

bool UnitSHT20::begin()
{
    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<sht20::Data>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    if (!softReset()) {
        M5_LIB_LOGE("Failed to reset");
        return false;
    }

    // Verify communication by reading user register
    uint8_t reg{};
    if (!read_user_register(reg)) {
        M5_LIB_LOGE("Failed to read user register");
        return false;
    }

    // Soft reset does NOT clear heater bit (datasheet: "except heater bit")
    auto r = _cfg.start_heater ? startHeater() : stopHeater();
    if (!r) {
        M5_LIB_LOGE("Failed to %s heater", _cfg.start_heater ? "start" : "stop");
        return false;
    }

    if (!writeResolution(_cfg.resolution)) {
        M5_LIB_LOGE("Failed to set resolution");
        return false;
    }

    return _cfg.start_periodic ? startPeriodicMeasurement() : true;
}

void UnitSHT20::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            sht20::Data d{};
            _updated = read_measurement(d);
            if (_updated) {
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool UnitSHT20::measureSingleshot(sht20::Data& d)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    return read_measurement(d);
}

bool UnitSHT20::start_periodic_measurement()
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    uint8_t idx            = resolution_index(_cfg.resolution);
    elapsed_time_t minimum = temp_duration[idx] + humd_duration[idx] + 10;  // margin
    _interval              = m5::stl::clamp(static_cast<elapsed_time_t>(_cfg.periodic_interval), minimum,
                                            static_cast<elapsed_time_t>(60000));
    _periodic              = true;
    return true;
}

bool UnitSHT20::stop_periodic_measurement()
{
    _periodic = false;
    _latest   = 0;
    return true;
}

bool UnitSHT20::read_measurement(sht20::Data& d)
{
    uint16_t temp_raw{}, humd_raw{};

    // Trigger temperature measurement (no hold master)
    if (!trigger_measurement(TRIGGER_TEMP_NOHOLD, temp_raw)) {
        return false;
    }
    d.raw[0] = static_cast<uint8_t>(temp_raw >> 8);
    d.raw[1] = static_cast<uint8_t>(temp_raw & 0xFF);
    d.raw[2] = 0;  // CRC already verified in trigger_measurement

    // Trigger humidity measurement (no hold master)
    if (!trigger_measurement(TRIGGER_HUMD_NOHOLD, humd_raw)) {
        return false;
    }
    d.raw[3] = static_cast<uint8_t>(humd_raw >> 8);
    d.raw[4] = static_cast<uint8_t>(humd_raw & 0xFF);
    d.raw[5] = 0;  // CRC already verified in trigger_measurement

    return true;
}

bool UnitSHT20::trigger_measurement(const uint8_t cmd, uint16_t& raw)
{
    // Send command
    if (writeWithTransaction(&cmd, 1) != m5::hal::error::error_t::OK) {
        return false;
    }

    // Determine wait time based on command and resolution
    uint8_t idx = resolution_index(_cfg.resolution);
    elapsed_time_t delay =
        (cmd == TRIGGER_TEMP_NOHOLD || cmd == TRIGGER_TEMP_HOLD) ? temp_duration[idx] : humd_duration[idx];

    m5::utility::delay(delay);

    // Read 3 bytes: MSB, LSB, CRC
    std::array<uint8_t, 3> buf{};
    if (readWithTransaction(buf.data(), buf.size()) != m5::hal::error::error_t::OK) {
        return false;
    }

    // Verify CRC
    if (crc_sht20.range(buf.data(), 2) != buf[2]) {
        M5_LIB_LOGE("CRC mismatch");
        return false;
    }

    raw = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
    return true;
}

bool UnitSHT20::readResolution(sht20::Resolution& res)
{
    uint8_t reg{};
    if (!read_user_register(reg)) {
        return false;
    }
    res = static_cast<sht20::Resolution>(reg & REG_RESOLUTION_MASK);
    return true;
}

bool UnitSHT20::writeResolution(const sht20::Resolution res)
{
    uint8_t reg{};
    if (!read_user_register(reg)) {
        return false;
    }
    reg = (reg & ~REG_RESOLUTION_MASK) | static_cast<uint8_t>(res);
    return write_user_register(reg);
}

bool UnitSHT20::startHeater()
{
    uint8_t reg{};
    if (!read_user_register(reg)) {
        return false;
    }
    reg |= REG_HEATER_BIT;
    return write_user_register(reg);
}

bool UnitSHT20::stopHeater()
{
    uint8_t reg{};
    if (!read_user_register(reg)) {
        return false;
    }
    reg &= ~REG_HEATER_BIT;
    return write_user_register(reg);
}

bool UnitSHT20::softReset()
{
    uint8_t cmd = SOFT_RESET;
    if (writeWithTransaction(&cmd, 1) != m5::hal::error::error_t::OK) {
        return false;
    }
    // Soft reset takes max 15ms
    m5::utility::delay(15);
    return true;
}

bool UnitSHT20::read_user_register(uint8_t& reg)
{
    uint8_t cmd = READ_USER_REG;
    if (writeWithTransaction(&cmd, 1) != m5::hal::error::error_t::OK) {
        return false;
    }
    if (readWithTransaction(&reg, 1) != m5::hal::error::error_t::OK) {
        return false;
    }
    return true;
}

bool UnitSHT20::write_user_register(const uint8_t reg)
{
    uint8_t buf[2] = {WRITE_USER_REG, reg};
    return writeWithTransaction(buf, 2) == m5::hal::error::error_t::OK;
}

}  // namespace unit
}  // namespace m5
