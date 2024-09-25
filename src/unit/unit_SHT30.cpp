/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SHT30.cpp
  @brief SHT30 Unit for M5UnitUnified
 */
#include "unit_SHT30.hpp"
#include <M5Utility.hpp>
#include <limits>  // NaN
#include <array>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::sht30;
using namespace m5::unit::sht30::command;

namespace {
struct Temperature {
    constexpr static float toFloat(const uint16_t u16)
    {
        return -45 + u16 * 175 / 65535.f;  // -45 + 175 * S / (2^16 - 1)
    }
};

// After sending a command to the sensor a minimalwaiting time of 1ms is needed
// before another commandcan be received by the sensor.
bool delay1()
{
    m5::utility::delay(1);
    return true;
}

constexpr uint16_t periodic_cmd[] = {
    // 0.5 mps
    START_PERIODIC_MPS_HALF_HIGH,
    START_PERIODIC_MPS_HALF_MEDIUM,
    START_PERIODIC_MPS_HALF_LOW,
    // 1 mps
    START_PERIODIC_MPS_1_HIGH,
    START_PERIODIC_MPS_1_MEDIUM,
    START_PERIODIC_MPS_1_LOW,
    // 2 mps
    START_PERIODIC_MPS_2_HIGH,
    START_PERIODIC_MPS_2_MEDIUM,
    START_PERIODIC_MPS_2_LOW,
    // 4 mps
    START_PERIODIC_MPS_4_HIGH,
    START_PERIODIC_MPS_4_MEDIUM,
    START_PERIODIC_MPS_4_LOW,
    // 10 mps
    START_PERIODIC_MPS_10_HIGH,
    START_PERIODIC_MPS_10_MEDIUM,
    START_PERIODIC_MPS_10_LOW,
};
constexpr elapsed_time_t interval_table[] = {
    2000,  // 0.5
    1000,  // 1
    500,   // 2
    250,   // 4
    100,   // 10
};

}  // namespace

namespace m5 {
namespace unit {
namespace sht30 {

float Data::celsius() const
{
    return Temperature::toFloat(m5::types::big_uint16_t(raw[0], raw[1]).get());
}

float Data::fahrenheit() const
{
    return celsius() * 9.0f / 5.0f + 32.f;
}

float Data::humidity() const
{
    return 100.f * m5::types::big_uint16_t(raw[3], raw[4]).get() / 65536.f;
}
}  // namespace sht30

const char UnitSHT30::name[] = "UnitSHT30";
const types::uid_t UnitSHT30::uid{"UnitSHT30"_mmh3};
const types::uid_t UnitSHT30::attr{0};

bool UnitSHT30::begin()
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

    if (!stopPeriodicMeasurement()) {
        M5_LIB_LOGE("Failed to stop");
        return false;
    }

    if (!softReset()) {
        M5_LIB_LOGE("Failed to reset");
        return false;
    }

    auto r = _cfg.start_heater ? startHeater() : stopHeater();
    if (!r) {
        M5_LIB_LOGE("Failed to heater %d", _cfg.start_heater);
        return false;
    }
    return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.mps, _cfg.repeatability) : true;
}

void UnitSHT30::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            if (writeRegister(READ_MEASUREMENT)) {
                Data d{};
                _updated = read_measurement(d);
                if (_updated) {
                    _latest = at;
                    _data->push_back(d);
                }
            }
        }
    }
}

bool UnitSHT30::measureSingleshot(Data& d, const sht30::Repeatability rep, const bool stretch)
{
    constexpr uint16_t cmd[] = {
        // Enable clock stretching
        SINGLE_SHOT_ENABLE_STRETCH_HIGH,
        SINGLE_SHOT_ENABLE_STRETCH_MEDIUM,
        SINGLE_SHOT_ENABLE_STRETCH_LOW,
        // Disable clock stretching
        SINGLE_SHOT_DISABLE_STRETCH_HIGH,
        SINGLE_SHOT_DISABLE_STRETCH_MEDIUM,
        SINGLE_SHOT_DISABLE_STRETCH_LOW,
    };
    // Latency when clock stretching is disabled
    constexpr elapsed_time_t ms[] = {
        15,
        6,
        4,
    };

    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    uint32_t idx = m5::stl::to_underlying(rep) + (stretch ? 0 : 3);
    if (idx >= m5::stl::size(cmd)) {
        M5_LIB_LOGE("Invalid arg : %u", (int)rep);
        return false;
    }

    if (writeRegister(cmd[idx])) {
        m5::utility::delay(stretch ? 1 : ms[m5::stl::to_underlying(rep)]);
        return read_measurement(d);
    }
    return false;
}

bool UnitSHT30::start_periodic_measurement(const sht30::MPS mps, const sht30::Repeatability rep)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    _periodic = writeRegister(periodic_cmd[m5::stl::to_underlying(mps) * 3 + m5::stl::to_underlying(rep)]);
    if (_periodic) {
        _interval = interval_table[m5::stl::to_underlying(mps)];
        m5::utility::delay(16);
        return true;
    }
    return _periodic;
}

bool UnitSHT30::stop_periodic_measurement()
{
    if (writeRegister(STOP_PERIODIC_MEASUREMENT)) {
        _periodic = false;
        _latest   = 0;
        // Upon reception of the break command the sensor will abort the
        // ongoing measurement and enter the single shot mode. This takes
        // 1ms
        return delay1();
    }
    return false;
}

bool UnitSHT30::writeModeAccelerateResponseTime()
{
    if (!inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are NOT running");
        return false;
    }
    if (writeRegister(ACCELERATED_RESPONSE_TIME)) {
        _interval = 1000 / 4;  // 4mps
        m5::utility::delay(16);
        return true;
    }
    return false;
}

bool UnitSHT30::readStatus(sht30::Status& s)
{
    std::array<uint8_t, 3> rbuf{};
    if (readRegister(READ_STATUS, rbuf.data(), rbuf.size(), 0) &&
        m5::utility::CRC8_Checksum().range(rbuf.data(), 2) == rbuf[2]) {
        s.value = m5::types::big_uint16_t(rbuf[0], rbuf[1]).get();
        return true;
    }
    return false;
}

bool UnitSHT30::clearStatus()
{
    return writeRegister(CLEAR_STATUS) && delay1();
}

bool UnitSHT30::softReset()
{
    if (inPeriodic()) {
        M5_LIB_LOGE("Periodic measurements are running");
        return false;
    }

    if (writeRegister(SOFT_RESET)) {
        // Max 1.5 ms
        // Time between ACK of soft reset command and sensor entering idle
        // state
        m5::utility::delay(2);
        return true;
    }
    return false;
}

bool UnitSHT30::generalReset()
{
    uint8_t cmd{0x06};  // reset command

    if (!clearStatus()) {
        return false;
    }
    // Reset does not return ACK, which is an error, but should be ignored
    generalCall(&cmd, 1);

    m5::utility::delay(1);

    auto timeout_at = m5::utility::millis() + 10;
    bool done{};
    do {
        Status s{};
        // The ALERT pin will also become active (high) after powerup and
        // after resets
        if (readStatus(s) && (s.reset() || s.alertPending())) {
            done = true;
            break;
        }
        m5::utility::delay(1);
    } while (!done && m5::utility::millis() <= timeout_at);
    return done;
}

bool UnitSHT30::startHeater()
{
    return writeRegister(START_HEATER) && delay1();
}

bool UnitSHT30::stopHeater()
{
    return writeRegister(STOP_HEATER) && delay1();
}

bool UnitSHT30::readSerialNumber(uint32_t& serialNumber)
{
    serialNumber = 0;
    if (inPeriodic()) {
        M5_LIB_LOGE("Periodic measurements are running");
        return false;
    }

    std::array<uint8_t, 6> rbuf;
    if (readRegister(GET_SERIAL_NUMBER_ENABLE_STRETCH, rbuf.data(), rbuf.size(), 0)) {
        m5::types::big_uint16_t u16[2]{{rbuf[0], rbuf[1]}, {rbuf[3], rbuf[4]}};
        m5::utility::CRC8_Checksum crc{};
        if (crc.range(u16[0].data(), u16[0].size()) == rbuf[2] && crc.range(u16[1].data(), u16[1].size()) == rbuf[5]) {
            serialNumber = ((uint32_t)u16[0].get()) << 16 | ((uint32_t)u16[1].get());
            return true;
        }
    }
    return false;
}

bool UnitSHT30::readSerialNumber(char* serialNumber)
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

bool UnitSHT30::read_measurement(Data& d)
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

}  // namespace unit
}  // namespace m5
