/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SHT40.cpp
  @brief SHT40 Unit for M5UnitUnified
 */
#include "unit_SHT40.hpp"
#include <M5Utility.hpp>
#include <limits>  // NaN
#include <array>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::sht40;
using namespace m5::unit::sht40::command;

namespace {
constexpr uint8_t periodic_cmd[] = {
    // HIGH
    MEASURE_HIGH_HEATER_1S,
    MEASURE_HIGH_HEATER_100MS,
    MEASURE_HIGH,
    // MEDIUM
    MEASURE_MEDIUM_HEATER_1S,
    MEASURE_MEDIUM_HEATER_100MS,
    MEASURE_MEDIUM,
    // LOW
    MEASURE_LOW_HEATER_1S,
    MEASURE_LOW_HEATER_100MS,
    MEASURE_LOW,
};

constexpr elapsed_time_t interval_table[] = {
    // HIGH
    1100, 110,
    9,  // 8.2
    // MEDIUM
    1100, 110,
    5,  // 4.5
    // LOW
    1100, 110,
    2,  // 1.7
};

constexpr float MAX_HEATER_DUTY{0.05f};
}  // namespace

namespace m5 {
namespace unit {
namespace sht40 {

float Data::celsius() const
{
    return -45 + 175 * m5::types::big_uint16_t(raw[0], raw[1]).get() / 65535.f;
}

float Data::fahrenheit() const
{
    return -49 + 315 * m5::types::big_uint16_t(raw[0], raw[1]).get() / 65535.f;
    //    return celsius() * 9.0f / 5.0f + 32.f;
}

float Data::humidity() const
{
    return -6 + 125 * m5::types::big_uint16_t(raw[3], raw[4]).get() / 65535.f;
}
}  // namespace sht40

const char UnitSHT40::name[] = "UnitSHT40";
const types::uid_t UnitSHT40::uid{"UnitSHT40"_mmh3};
const types::attr_t UnitSHT40::attr{attribute::AccessI2C};

bool UnitSHT40::begin()
{
    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<Data>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    if (!softReset()) {
        M5_LIB_LOGE("Failed to reset");
        return false;
    }

    uint32_t sn{};
    if (!readSerialNumber(sn)) {
        M5_LIB_LOGE("Failed to readSerialNumber %x", sn);
        return false;
    }

    return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.precision, _cfg.heater, _cfg.heater_duty) : true;
}

void UnitSHT40::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = read_measurement(d);

            if (_updated) {
                _latest  = at;
                d.heater = (_interval != _duration_heater);
                _data->push_back(d);

                uint8_t cmd{};
                if (at >= _latest_heater + _interval_heater) {
                    cmd            = _cmd;
                    _latest_heater = at;
                    _interval      = _duration_heater;
                } else {
                    cmd       = _measureCmd;
                    _interval = _duration_measure;
                }
                if (!writeRegister(cmd)) {
                    M5_LIB_LOGE("Failed to write, stop periodic measurement");
                    _periodic = false;
                }
            }
        }
    }
}

bool UnitSHT40::start_periodic_measurement(const sht40::Precision precision, const sht40::Heater heater,
                                           const float duty)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (duty <= 0.0f || duty > MAX_HEATER_DUTY) {
        M5_LIB_LOGW("duty range is invalid %f. duty (0.0, 0.05]");
        return false;
    }

    _cmd        = periodic_cmd[m5::stl::to_underlying(precision) * 3 + m5::stl::to_underlying(heater)];
    _measureCmd = periodic_cmd[m5::stl::to_underlying(precision) * 3 + m5::stl::to_underlying(Heater::None)];

    _periodic = writeRegister(_cmd);
    if (_periodic) {
        _duration_heater = interval_table[m5::stl::to_underlying(precision) * 3 + m5::stl::to_underlying(heater)];
        _duration_measure =
            interval_table[m5::stl::to_underlying(precision) * 3 + m5::stl::to_underlying(Heater::None)];

        _interval_heater = _duration_heater / duty;
        _interval        = _duration_heater;
        _latest_heater   = m5::utility::millis();

        m5::utility::delay(_interval);  // For first read_measurement in update
        return true;
    }
    return _periodic;
}

bool UnitSHT40::stop_periodic_measurement()
{
    if (inPeriodic()) {
        // Dismissal of data to be read
        Data discard{};
        int64_t wait = (int64_t)(_latest + _interval) - (int64_t)m5::utility::millis();
        if (wait > 0) {
            m5::utility::delay(wait);
            read_measurement(discard);
        }

        _periodic = false;
        return true;
    }
    return false;
}

bool UnitSHT40::measureSingleshot(sht40::Data& d, const sht40::Precision precision, const sht40::Heater heater)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    uint8_t cmd = periodic_cmd[m5::stl::to_underlying(precision) * 3 + m5::stl::to_underlying(heater)];
    auto ms     = interval_table[m5::stl::to_underlying(precision) * 3 + m5::stl::to_underlying(heater)];

    if (writeRegister(cmd)) {
        m5::utility::delay(ms);
        if (read_measurement(d)) {
            d.heater = (heater != Heater::None);
            return true;
        }
    }
    return false;
}

bool UnitSHT40::read_measurement(sht40::Data& d)
{
    if (readWithTransaction(d.raw.data(), d.raw.size()) == m5::hal::error::error_t::OK) {
        m5::utility::CRC8_Checksum crc{};
        for (uint_fast8_t i = 0; i < 2; ++i) {
            if (crc.range(d.raw.data() + i * 3, 2U) != d.raw[i * 3 + 2]) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool UnitSHT40::softReset()
{
    if (inPeriodic()) {
        M5_LIB_LOGE("Periodic measurements are running");
        return false;
    }
    return soft_reset();
}

bool UnitSHT40::soft_reset()
{
    if (writeRegister(SOFT_RESET)) {
        // Max 1 ms
        // Time between ACK of soft reset command and sensor entering idle state
        m5::utility::delay(1);
        reset_status();
        return true;
    }
    return false;
}

bool UnitSHT40::generalReset()
{
    uint8_t cmd{0x06};  // reset command
    // Reset does not return ACK, which is an error, but should be ignored
    generalCall(&cmd, 1);

    m5::utility::delay(1);
    reset_status();

    return true;
}

bool UnitSHT40::readSerialNumber(uint32_t& serialNumber)
{
    serialNumber = 0;
    if (inPeriodic()) {
        M5_LIB_LOGE("Periodic measurements are running");
        return false;
    }

    std::array<uint8_t, 6> rbuf;
    if (readRegister(GET_SERIAL_NUMBER, rbuf.data(), rbuf.size(), 1)) {
        m5::types::big_uint16_t u16[2]{{rbuf[0], rbuf[1]}, {rbuf[3], rbuf[4]}};
        m5::utility::CRC8_Checksum crc{};
        if (crc.range(u16[0].data(), u16[0].size()) == rbuf[2] && crc.range(u16[1].data(), u16[1].size()) == rbuf[5]) {
            serialNumber = ((uint32_t)u16[0].get()) << 16 | ((uint32_t)u16[1].get());
            return true;
        }
    }
    return false;
}

bool UnitSHT40::readSerialNumber(char* serialNumber)
{
    if (!serialNumber) {
        return false;
    }

    *serialNumber = '\0';
    uint32_t sno{};
    if (readSerialNumber(sno)) {
        uint_fast8_t i{8};
        while (i--) {
            *serialNumber++ = m5::utility::uintToHexChar((sno >> (i * 4)) & 0x0F);
        }
        *serialNumber = '\0';
        return true;
    }
    return false;
}

void UnitSHT40::reset_status()
{
    _interval = _latest = _interval_heater = _latest_heater = 0;
    _duration_measure = _duration_heater = 0;
    _cmd = _measureCmd = 0;
    _periodic          = false;
}

}  // namespace unit
}  // namespace m5
