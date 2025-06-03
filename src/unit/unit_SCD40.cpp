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

const uint8_t VARIANT_VALUE[2]{0x04, 0x40};  // SCD40

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
const types::attr_t UnitSCD40::attr{attribute::AccessI2C};

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

    // Stop (to idle mode)
    if (!writeRegister(STOP_PERIODIC_MEASUREMENT)) {
        M5_LIB_LOGE("Failed to stop");
        return false;
    }
    m5::utility::delay(STOP_PERIODIC_MEASUREMENT_DURATION);

    if (!is_valid_chip()) {
        return false;
    }

    if (!writeAutomaticSelfCalibrationEnabled(_cfg.calibration)) {
        M5_LIB_LOGE("Failed to writeAutomaticSelfCalibrationEnabled");
        return false;
    }

    // Stop

    return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.mode) : true;
}

bool UnitSCD40::is_valid_chip()
{
    uint8_t var[2]{};
    if (!read_register(GET_SENSOR_VARIANT, var, 2) || memcmp(var, VARIANT_VALUE, 2) != 0) {
        M5_LIB_LOGE("Not SCD40 %02X:%02X", var[0], var[1]);
        return false;
    }
    return true;
}

void UnitSCD40::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        auto at = m5::utility::millis();
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = read_measurement(d);
            if (_updated) {
                _latest = m5::utility::millis();  // Data acquisition takes time, so acquire again
                _data->push_back(d);
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
    if (inPeriodic()) {
        if (writeRegister(STOP_PERIODIC_MEASUREMENT)) {
            _periodic = false;
            m5::utility::delay(duration);
            return true;
        }
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

    uint8_t wbuf[2]{};
    uint16_t tmp16 = Temperature::toUint16(offset);
    wbuf[0]        = tmp16 >> 8;
    wbuf[1]        = tmp16 & 0xFF;
    return write_register(SET_TEMPERATURE_OFFSET, wbuf, sizeof(wbuf)) && delay_true(duration);
}

bool UnitSCD40::readTemperatureOffset(float& offset)
{
    offset = 0.0f;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    m5::types::big_uint16_t u16{};
    if (read_register(GET_TEMPERATURE_OFFSET, u16.data(), u16.size(), GET_TEMPERATURE_OFFSET_DURATION)) {
        offset = Temperature::toFloat(u16.get());
        return true;
    }
    return false;
}

bool UnitSCD40::writeSensorAltitude(const uint16_t altitude, const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    m5::types::big_uint16_t u16(altitude);
    return write_register(SET_SENSOR_ALTITUDE, u16.data(), u16.size()) && delay_true(duration);
}

bool UnitSCD40::readSensorAltitude(uint16_t& altitude)
{
    altitude = 0;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    m5::types::big_uint16_t u16{};
    if (read_register(GET_SENSOR_ALTITUDE, u16.data(), u16.size(), GET_SENSOR_ALTITUDE_DURATION)) {
        altitude = u16.get();
        return true;
    }
    return false;
}

bool UnitSCD40::writeAmbientPressure(const uint16_t pressure, const uint32_t duration)
{
    constexpr uint32_t PRESSURE_MIN{700};
    constexpr uint32_t PRESSURE_MAX{1200};

#if 0
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
#endif
    if (pressure < PRESSURE_MIN || pressure > PRESSURE_MAX) {
        M5_LIB_LOGE("pressure is not a valid scope (%u - %u) %u", PRESSURE_MIN, PRESSURE_MAX, pressure);
        return false;
    }
    m5::types::big_uint16_t u16(pressure);
    return write_register(AMBIENT_PRESSURE, u16.data(), u16.size()) && delay_true(duration);
}

bool UnitSCD40::readAmbientPressure(uint16_t& pressure)
{
    pressure = 0;
#if 0
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
#endif
    m5::types::big_uint16_t u16{};
    if (read_register(AMBIENT_PRESSURE, u16.data(), u16.size(), GET_AMBIENT_PRESSURE_DURATION)) {
        pressure = u16.get();
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
    if (!write_register(PERFORM_FORCED_CALIBRATION, u16.data(), u16.size())) {
        return false;
    }
#if 0

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
#else

    // 3. Subsequently issue the perform_forced_recalibration command and
    // optionally read out the FRC correction (i.e. the magnitude of the
    // correction) after waiting for 400 ms for the command to complete.
    m5::utility::delay(PERFORM_FORCED_CALIBRATION_DURATION);

    if (read_register(PERFORM_FORCED_CALIBRATION, u16.data(), u16.size()) && u16.get() != 0xFFFF) {
        correction = (int16_t)(u16.get() - 0x8000);
        return true;
    }
    return false;
#endif
}

bool UnitSCD40::writeAutomaticSelfCalibrationEnabled(const bool enabled, const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    m5::types::big_uint16_t u16(enabled ? 0x0001 : 0x0000);
    return write_register(SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, u16.data(), u16.size()) && delay_true(duration);
}

bool UnitSCD40::readAutomaticSelfCalibrationEnabled(bool& enabled)
{
    enabled = false;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    m5::types::big_uint16_t u16{};
    if (read_register(GET_AUTOMATIC_SELF_CALIBRATION_ENABLED, u16.data(), u16.size(),
                      GET_AUTOMATIC_SELF_CALIBRATION_ENABLED_DURATION)) {
        enabled = (u16.get() == 0x0001);
        return true;
    }
    return false;
}

bool UnitSCD40::writeAutomaticSelfCalibrationTarget(const uint16_t ppm, const uint32_t duration)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    m5::types::big_uint16_t u16{ppm};
    return write_register(SET_AUTOMATIC_SELF_CALIBRATION_TARGET, u16.data(), u16.size()) && delay_true(duration);
}

bool UnitSCD40::readAutomaticSelfCalibrationTarget(uint16_t& ppm)
{
    ppm = 0;
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    m5::types::big_uint16_t u16{};
    if (read_register(GET_AUTOMATIC_SELF_CALIBRATION_TARGET, u16.data(), u16.size(),
                      GET_AUTOMATIC_SELF_CALIBRATION_TARGET_DURATION)) {
        ppm = u16.get();
        return true;
    }
    return false;
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
    malfunction = true;

    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    m5::types::big_uint16_t u16{};
    if (read_register(PERFORM_SELF_TEST, u16.data(), u16.size(), PERFORM_SELF_TEST_DURATION)) {
        malfunction = (u16.get() != 0);
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

bool UnitSCD40::read_data_ready_status()
{
    uint16_t res{};
    return readRegister16BE(GET_DATA_READY_STATUS, res, GET_DATA_READY_STATUS_DURATION) ? (res & 0x07FF) != 0 : false;
}

// TH only if all is false
bool UnitSCD40::read_measurement(Data& d, const bool all)
{
    if (!read_data_ready_status()) {
        M5_LIB_LOGV("Not ready");
        return false;
    }
    if (!readRegister(READ_MEASUREMENT, d.raw.data(), d.raw.size(), READ_MEASUREMENT_DURATION)) {
        return false;
    }

    // For RHT only, previous Co2 data may be obtained and should be dismissed
    if (!all) {
        d.raw[0] = d.raw[1] = d.raw[2] = 0;
    }

    // Check CRC
    m5::utility::CRC8_Checksum crc{};
    for (uint_fast8_t i = all ? 0 : 1; i < 3; ++i) {
        if (crc.range(d.raw.data() + i * 3, 2U) != d.raw[i * 3 + 2]) {
            return false;
        }
    }
    return true;
}

bool UnitSCD40::read_register(const uint16_t reg, uint8_t* rbuf, const uint32_t rlen, const uint32_t duration)
{
    uint8_t tmp[rlen + 1]{};
    if (!rbuf || !rlen || !readRegister(reg, tmp, sizeof(tmp), duration)) {
        return false;
    }

    m5::utility::CRC8_Checksum crc{};
    auto crc8 = crc.range(tmp, rlen);
    if (crc8 != tmp[rlen]) {
        M5_LIB_LOGE("CRC8 Error:%02X, %02X", tmp[rlen], crc8);
        return false;
    }
    memcpy(rbuf, tmp, rlen);
    return true;
}

bool UnitSCD40::write_register(const uint16_t reg, uint8_t* wbuf, const uint32_t wlen)
{
    uint8_t buf[wlen + 1]{};
    if (!wbuf || !wlen) {
        return false;
    }
    memcpy(buf, wbuf, wlen);
    m5::utility::CRC8_Checksum crc{};
    auto crc8 = crc.range(wbuf, wlen);
    buf[wlen] = crc8;
    return writeRegister(reg, buf, sizeof(buf));
}

bool UnitSCD40::delay_true(const uint32_t duration)
{
    m5::utility::delay(duration);
    return true;  // Always true
}

}  // namespace unit
}  // namespace m5
