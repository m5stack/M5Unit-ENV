/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SCD40.cpp
  @brief SCD40 Unit for M5UnitUnified
*/
#include "unit_SCD40.hpp"
#include <M5Utility.hpp>
#include <array>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::scd4x;
using namespace m5::unit::scd4x::command;

namespace {
struct Temperature {
    constexpr static float toFloat(const uint16_t u16)
    {
        return u16 * 175.f / 65536.f;
    }
    constexpr static uint16_t toUint16(const float f)
    {
        return f * 65536 / 175;
    }
    constexpr static float OFFSET_MIN{0.0f};
    constexpr static float OFFSET_MAX{175.0f};
};

constexpr uint16_t mode_reg_table[] = {
    START_PERIODIC_MEASUREMENT,
    START_LOW_POWER_PERIODIC_MEASUREMENT,
};

constexpr uint32_t interval_table[] = {
    5000U,       // 5 Sec.
    30 * 1000U,  // 30 Sec.
};

}  // namespace

namespace m5 {
namespace unit {

namespace scd4x {
uint16_t Data::co2() const
{
    return m5::types::big_uint16_t(raw[0], raw[1]).get();
}

float Data::celsius() const
{
    return -45 + Temperature::toFloat(m5::types::big_uint16_t(raw[3], raw[4]).get());
}

float Data::fahrenheit() const
{
    return celsius() * 9.0f / 5.0f + 32.f;
}

float Data::humidity() const
{
    return 100.f * m5::types::big_uint16_t(raw[6], raw[7]).get() / 65536.f;
}

}  // namespace scd4x

// class UnitSCD40
const char UnitSCD40::name[] = "UnitSCD40";
const types::uid_t UnitSCD40::uid{"UnitSCD40"_mmh3};
const types::uid_t UnitSCD40::attr{0};

bool UnitSCD40::begin()
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

    if (!writeAutomaticSelfCalibrationEnabled(_cfg.calibration)) {
        M5_LIB_LOGE("Failed to write automatic calibration");
        return false;
    }

    return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.mode) : true;
}

void UnitSCD40::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        unsigned long at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = read_measurement(d);
            if (_updated) {
                _data->push_back(d);
                _latest = at;
            }
        }
    }
}

bool UnitSCD40::start_periodic_measurement(const Mode mode)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    auto m    = m5::stl::to_underlying(mode);
    _periodic = writeRegister(mode_reg_table[m]);
    if (_periodic) {
        _interval = interval_table[m];
        _latest   = 0;
    }
    return _periodic;
}

bool UnitSCD40::stop_periodic_measurement(const uint32_t duration)
{
    if (writeRegister(STOP_PERIODIC_MEASUREMENT)) {
        _periodic = false;
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitSCD40::writeTemperatureOffset(const float offset, const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (offset < Temperature::OFFSET_MIN || offset >= Temperature::OFFSET_MAX) {
        M5_LIB_LOGE("offset is not a valid scope %f", offset);
        return false;
    }

    m5::types::big_uint16_t u16(Temperature::toUint16(offset));
    m5::utility::CRC8_Checksum crc{};
    std::array<uint8_t, 3> buf{u16.u8[0], u16.u8[1], crc.range(u16.data(), u16.size())};
    if (writeRegister(SET_TEMPERATURE_OFFSET, buf.data(), buf.size())) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitSCD40::readTemperatureOffset(float& offset)
{
    offset = 0.0f;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    uint16_t u16{};
    auto ret = readRegister16(GET_TEMPERATURE_OFFSET, u16, GET_TEMPERATURE_OFFSET_DURATION);
    offset   = Temperature::toFloat(u16);
    return ret;
}

bool UnitSCD40::writeSensorAltitude(const uint16_t altitude, const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    m5::types::big_uint16_t u16(altitude);
    m5::utility::CRC8_Checksum crc{};
    std::array<uint8_t, 3> buf{u16.u8[0], u16.u8[1], crc.range(u16.data(), u16.size())};
    if (writeRegister(SET_SENSOR_ALTITUDE, buf.data(), buf.size())) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitSCD40::readSensorAltitude(uint16_t& altitude)
{
    altitude = 0;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    return readRegister16(GET_SENSOR_ALTITUDE, altitude, GET_SENSOR_ALTITUDE_DURATION);
}

bool UnitSCD40::writeAmbientPressure(const float pressure, const uint32_t duration)
{
    if (pressure < 0.0f || pressure > 65535.f * 100) {
        M5_LIB_LOGE("pressure is not a valid scope %f", pressure);
        return false;
    }
    m5::types::big_uint16_t u16((uint16_t)(pressure / 100));
    m5::utility::CRC8_Checksum crc{};
    std::array<uint8_t, 3> buf{u16.u8[0], u16.u8[1], crc.range(u16.data(), u16.size())};
    if (writeRegister(SET_AMBIENT_PRESSURE, buf.data(), buf.size())) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitSCD40::performForcedRecalibration(const uint16_t concentration, int16_t& correction)
{
    // 1. Operate the SCD4x in the operation mode later used in normal sensor
    // operation (periodic measurement, low power periodic measurement or single
    // shot) for > 3 minutes in an environment with homogenous and constant CO2
    // concentration.
    // 2. Issue stop_periodic_measurement. Wait 500 ms for the stop command to
    // complete.
    correction = 0;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    m5::types::big_uint16_t u16(concentration);
    m5::utility::CRC8_Checksum crc{};
    std::array<uint8_t, 3> buf{u16.u8[0], u16.u8[1], crc.range(u16.data(), u16.size())};
    if (!writeRegister(PERFORM_FORCED_CALIBRATION, buf.data(), buf.size())) {
        return false;
    }

    // 3. Subsequently issue the perform_forced_recalibration command and
    // optionally read out the FRC correction (i.e. the magnitude of the
    // correction) after waiting for 400 ms for the command to complete.
    m5::utility::delay(PERFORM_FORCED_CALIBRATION_DURATION);

    std::array<uint8_t, 3> rbuf{};
    if (readWithTransaction(rbuf.data(), rbuf.size()) == m5::hal::error::error_t::OK) {
        m5::types::big_uint16_t u16{rbuf[0], rbuf[1]};
        m5::utility::CRC8_Checksum crc{};
        if (rbuf[2] == crc.range(u16.data(), u16.size()) && u16.get() != 0xFFFF) {
            correction = (int16_t)(u16.get() - 0x8000);
            return true;
        }
    }
    return false;
}

bool UnitSCD40::writeAutomaticSelfCalibrationEnabled(const bool enabled, const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    m5::types::big_uint16_t u16(enabled ? 0x0001 : 0x0000);
    m5::utility::CRC8_Checksum crc{};
    std::array<uint8_t, 3> buf{u16.u8[0], u16.u8[1], crc.range(u16.data(), u16.size())};
    if (writeRegister(SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, buf.data(), buf.size())) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitSCD40::readAutomaticSelfCalibrationEnabled(bool& enabled)
{
    enabled = false;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    uint16_t u16{};
    if (readRegister16(GET_AUTOMATIC_SELF_CALIBRATION_ENABLED, u16, 1)) {
        enabled = (u16 == 0x0001);
        return true;
    }
    return false;
}

bool UnitSCD40::read_data_ready_status()
{
    uint16_t res{};
    return readRegister16(GET_DATA_READY_STATUS, res, GET_DATA_READY_STATUS_DURATION) ? (res & 0x07FF) != 0 : false;
}

bool UnitSCD40::writePersistSettings(const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    if (writeRegister(PERSIST_SETTINGS)) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitSCD40::readSerialNumber(char* serialNumber)
{
    if (!serialNumber) {
        return false;
    }

    *serialNumber = '\0';
    uint64_t sno{};
    if (readSerialNumber(sno)) {
        uint_fast8_t i{12};
        while (i--) {
            *serialNumber++ = m5::utility::uintToHexChar((sno >> (i * 4)) & 0x0F);
        }
        *serialNumber = '\0';
        return true;
    }
    return false;
}

bool UnitSCD40::readSerialNumber(uint64_t& serialNumber)
{
    std::array<uint8_t, 9> rbuf;
    serialNumber = 0;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    m5::utility::CRC8_Checksum crc{};
    if (readRegister(GET_SERIAL_NUMBER, rbuf.data(), rbuf.size(), GET_SERIAL_NUMBER_DURATION)) {
        m5::types::big_uint16_t u16[3]{{rbuf[0], rbuf[1]}, {rbuf[3], rbuf[4]}, {rbuf[6], rbuf[7]}};
        if (crc.range(u16[0].data(), u16[0].size()) == rbuf[2] && crc.range(u16[1].data(), u16[1].size()) == rbuf[5] &&
            crc.range(u16[2].data(), u16[2].size()) == rbuf[8]) {
            serialNumber = ((uint64_t)u16[0].get()) << 32 | ((uint64_t)u16[1].get()) << 16 | ((uint64_t)u16[2].get());
            return true;
        }
    }
    return false;
}

bool UnitSCD40::performSelfTest(bool& malfunction)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    uint16_t response{};
    if (readRegister16(PERFORM_SELF_TEST, response, PERFORM_SELF_TEST_DURATION)) {
        malfunction = (response != 0);
        return true;
    }
    return false;
}

bool UnitSCD40::performFactoryReset(const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (writeRegister(PERFORM_FACTORY_RESET)) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitSCD40::reInit(const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    if (writeRegister(REINIT)) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

// TH only if false
bool UnitSCD40::read_measurement(Data& d, const bool all)
{
    if (!read_data_ready_status()) {
        M5_LIB_LOGV("Not ready");
        return false;
    }
    if (!readRegister(READ_MEASUREMENT, d.raw.data(), d.raw.size(), READ_MEASUREMENT_DURATION)) {
        return false;
    }
    m5::utility::CRC8_Checksum crc{};
    for (uint_fast8_t i = all ? 0 : 1; i < 3; ++i) {
        if (crc.range(d.raw.data() + i * 3, 2U) != d.raw[i * 3 + 2]) {
            return false;
        }
    }
    return true;
}
}  // namespace unit
}  // namespace m5
