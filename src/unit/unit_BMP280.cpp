/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_BMP280.cpp
  @brief BMP280 Unit for M5UnitUnified
 */
#include "unit_BMP280.hpp"
#include <M5Utility.hpp>
#include <limits>  // NaN
#include <array>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::bmp280;
using namespace m5::unit::bmp280::command;

namespace {

constexpr uint8_t CHIP_IDENTIFIER{0x58};
constexpr uint8_t RESET_VALUE{0xB6};
constexpr uint32_t NOT_MEASURED{0x800000};

constexpr PowerMode mode_table[] = {PowerMode::Sleep, PowerMode::Forced, PowerMode::Forced,  // duplicated
                                    PowerMode::Normal

};

constexpr Oversampling osrs_table[] = {
    Oversampling::Skipped, Oversampling::X1, Oversampling::X2, Oversampling::X4, Oversampling::X8, Oversampling::X16,
    Oversampling::X16,  // duplicated
    Oversampling::X16,  // duplicated
};

constexpr Oversampling osrss_table[][2] = {
    // Pressure, Temperature
    {Oversampling::X1, Oversampling::X1}, {Oversampling::X2, Oversampling::X1},  {Oversampling::X4, Oversampling::X1},
    {Oversampling::X8, Oversampling::X1}, {Oversampling::X16, Oversampling::X2},
};

constexpr Standby standby_table[] = {
    Standby::Time0_5ms, Standby::Time62_5ms, Standby::Time125ms, Standby::Time250ms,
    Standby::Time500ms, Standby::Time1sec,   Standby::Time2sec,  Standby::Time4sec,

};

constexpr uint32_t interval_table[] = {0, 62, 125, 250, 500, 1000, 2000, 4000};

constexpr Filter filter_table[] = {
    Filter::Off, Filter::Coeff2, Filter::Coeff4, Filter::Coeff8, Filter::Coeff16,
};

struct UseCaseSetting {
    OversamplingSetting osrss;
    Filter filter;
    Standby st;
};
constexpr UseCaseSetting uc_table[] = {
    {OversamplingSetting::UltraHighResolution, Filter::Coeff4, Standby::Time62_5ms},
    {OversamplingSetting::StandardResolution, Filter::Coeff16, Standby::Time0_5ms},
    {OversamplingSetting::UltraLowPower, Filter::Off, Standby::Time4sec},
    {OversamplingSetting::StandardResolution, Filter::Coeff4, Standby::Time125ms},
    {OversamplingSetting::LowPower, Filter::Off, Standby::Time0_5ms},
    {OversamplingSetting::UltraHighResolution, Filter::Coeff16, Standby::Time0_5ms},
};

struct CtrlMeas {
    //
    inline Oversampling osrs_p() const
    {
        return osrs_table[(value >> 2) & 0x07];
    }
    inline Oversampling osrs_t() const
    {
        return osrs_table[(value >> 5) & 0x07];
    }
    inline PowerMode mode() const
    {
        return mode_table[value & 0x03];
    }
    //
    inline void osrs_p(const Oversampling os)
    {
        value = (value & ~(0x07 << 2)) | ((m5::stl::to_underlying(os) & 0x07) << 2);
    }
    inline void osrs_t(const Oversampling os)
    {
        value = (value & ~(0x07 << 5)) | ((m5::stl::to_underlying(os) & 0x07) << 5);
    }
    inline void mode(const PowerMode m)
    {
        value = (value & ~0x03) | (m5::stl::to_underlying(m) & 0x03);
    }
    uint8_t value{};
};

struct Config {
    //
    inline Standby standby() const
    {
        return standby_table[(value >> 5) & 0x07];
    }
    inline Filter filter() const
    {
        return filter_table[(value >> 2) & 0x07];
    }
    //
    inline void standby(const Standby s)
    {
        value = (value & ~(0x07 << 5)) | ((m5::stl::to_underlying(s) & 0x07) << 5);
    }
    inline void filter(const Filter f)
    {
        value = (value & ~(0x07 << 2)) | ((m5::stl::to_underlying(f) & 0x07) << 2);
    }
    uint8_t value{};
};

struct Calculator {
    inline float temperature(const int32_t adc_P, const int32_t adc_T, const Trimming* t)
    {
        return t ? compensate_temperature_f(adc_T, *t) : std::numeric_limits<float>::quiet_NaN();
    }
    inline float pressure(const int32_t adc_P, const int32_t adc_T, const Trimming* t)
    {
        if (!t) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        if (t_fine == 0) {
            (void)compensate_temperature_f(adc_T, *t);  // For t_fine
        }
        return compensate_pressure_f(adc_P, *t);
    }

private:
    float compensate_temperature(const int32_t adc_T, const Trimming& trim)
    {
        int32_t var1{}, var2{};
        var1 = ((((adc_T >> 3) - ((int32_t)trim.dig_T1 << 1))) * ((int32_t)trim.dig_T2)) >> 11;
        var2 = (((((adc_T >> 4) - ((int32_t)trim.dig_T1)) * ((adc_T >> 4) - ((int32_t)trim.dig_T1))) >> 12) *
                ((int32_t)trim.dig_T3)) >>
               14;
        t_fine  = var1 + var2;  // [*1]
        float T = (t_fine * 5 + 128) >> 8;
        return T * 0.01f;
    }

    float compensate_pressure(const int32_t adc_P, const Trimming& trim)
    {
        int64_t var1{}, var2{}, p{};
        var1 = ((int64_t)t_fine) - 128000;  // (*1) using it!
        var2 = var1 * var1 * (int64_t)trim.dig_P6;
        var2 = var2 + ((var1 * (int64_t)trim.dig_P5) << 17);
        var2 = var2 + (((int64_t)trim.dig_P4) << 35);
        var1 = ((var1 * var1 * (int64_t)trim.dig_P3) >> 8) + ((var1 * (int64_t)trim.dig_P2) << 12);
        var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)trim.dig_P1) >> 33;
        if (var1) {
            p    = 1048576 - adc_P;
            p    = (((p << 31) - var2) * 3125) / var1;
            var1 = (((int64_t)trim.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
            var2 = (((int64_t)trim.dig_P8) * p) >> 19;
            p    = ((p + var1 + var2) >> 8) + (((int64_t)trim.dig_P7) << 4);
            return p / 256.f;
        }
        return 0.0f;
    }

    float compensate_temperature_f(const int32_t adc_T, const Trimming& trim)
    {
        float var1{}, var2{}, T{};
        var1 = (((float)adc_T) / 16384.0f - ((float)trim.dig_T1) / 1024.0f) * ((float)trim.dig_T2);
        var2 = ((((float)adc_T) / 131072.0f - ((float)trim.dig_T1) / 8192.0f) *
                (((float)adc_T) / 131072.0f - ((float)trim.dig_T1) / 8192.0f)) *
               ((float)trim.dig_T3);
        t_fine = (int32_t)(var1 + var2);  // [*2]
        T      = (var1 + var2) / 5120.0f;
        return T;
    }

    float compensate_pressure_f(const int32_t adc_P, const Trimming& trim)
    {
        float var1{}, var2{}, P{};
        var1 = ((float)t_fine / 2.0f) - 64000.0f;  // (*2)
        var2 = var1 * var1 * ((float)trim.dig_P6) / 32768.0f;
        var2 = var2 + var1 * ((float)trim.dig_P5) * 2.0f;
        var2 = (var2 / 4.0f) + (((float)trim.dig_P4) * 65536.0f);
        var1 = (((float)trim.dig_P3) * var1 * var1 / 524288.0f + ((float)trim.dig_P2) * var1) / 524288.0f;
        var1 = (1.0f + var1 / 32768.0f) * ((float)trim.dig_P1);
        if (var1 == 0.0f) {
            return 0;
        }
        P    = 1048576.0f - (float)adc_P;
        P    = (P - (var2 / 4096.0f)) * 6250.0f / var1;
        var1 = ((float)trim.dig_P9) * P * P / 2147483648.0f;
        var2 = P * ((float)trim.dig_P8) / 32768.0f;
        P    = P + (var1 + var2 + ((float)trim.dig_P7)) / 16.0f;
        return P;
    }

    ////

    int32_t t_fine{};
};
}  // namespace

namespace m5 {
namespace unit {
namespace bmp280 {

float Data::celsius() const
{
    int32_t adc_P = (int32_t)(((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2]));
    int32_t adc_T = (int32_t)(((uint32_t)raw[3] << 16) | ((uint32_t)raw[4] << 8) | ((uint32_t)raw[5]));
    Calculator c{};

    // adc_T is NOT_MEASURED if orsr Skipped (Not measured)
    return (adc_T != NOT_MEASURED) ? c.temperature(adc_P >> 4, adc_T >> 4, trimming)
                                   : std::numeric_limits<float>::quiet_NaN();
}

float Data::fahrenheit() const
{
    return celsius() * 9.0f / 5.0f + 32.f;
}

float Data::pressure() const
{
    int32_t adc_P = (int32_t)(((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2]));
    int32_t adc_T = (int32_t)(((uint32_t)raw[3] << 16) | ((uint32_t)raw[4] << 8) | ((uint32_t)raw[5]));
    Calculator c{};

    // adc_T/P is NOT_MEASURED if orsr Skipped (Not measured)
    return (adc_T != NOT_MEASURED && adc_P != NOT_MEASURED) ? c.pressure(adc_P >> 4, adc_T >> 4, trimming)
                                                            : std::numeric_limits<float>::quiet_NaN();
}

}  // namespace bmp280

const char UnitBMP280::name[] = "UnitBMP280";
const types::uid_t UnitBMP280::uid{"UnitBMP280"_mmh3};
const types::attr_t UnitBMP280::attr{attribute::AccessI2C};

bool UnitBMP280::begin()
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

    uint8_t id{};
    if (!softReset() || !readRegister8(CHIP_ID, id, 0) || id != CHIP_IDENTIFIER) {
        M5_LIB_LOGE("Can not detect BMP280 %02X", id);
        return false;
    }

    if (!read_trimming(_trimming)) {
        M5_LIB_LOGE("Failed to read trimming");
        return false;
    }

    M5_LIB_LOGV(
        "Trimming\n"
        "T:%u/%d/%d\n"
        "P:%u/%d/%d/%d/%d/%d/%d/%d/%d",
        // T
        _trimming.dig_T1, _trimming.dig_T2, _trimming.dig_T3,
        // P
        _trimming.dig_P1, _trimming.dig_P2, _trimming.dig_P3, _trimming.dig_P4, _trimming.dig_P5, _trimming.dig_P6,
        _trimming.dig_P7, _trimming.dig_P8, _trimming.dig_P9);

    return _cfg.start_periodic
               ? startPeriodicMeasurement(_cfg.osrs_pressure, _cfg.osrs_temperature, _cfg.filter, _cfg.standby)
               : true;
}

void UnitBMP280::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            //            _updated = is_data_ready() && read_measurement(d);
            _updated = read_measurement(d);
            if (_updated) {
                // auto dur = at - _latest;
                // M5_LIB_LOGW(">DUR:%ld\n", dur);
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool UnitBMP280::start_periodic_measurement(const bmp280::Oversampling osrsPressure,
                                            const bmp280::Oversampling osrsTemperature, const bmp280::Filter filter,
                                            const bmp280::Standby st)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Config c{};
    c.standby(st);
    c.filter(filter);
    CtrlMeas cm{};
    cm.osrs_p(osrsPressure);
    cm.osrs_t(osrsTemperature);

    return writeRegister8(CONFIG, c.value) && writeRegister8(CONTROL_MEASUREMENT, cm.value) &&
           start_periodic_measurement();
}

bool UnitBMP280::start_periodic_measurement()
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Config c{};
    _periodic = readRegister8(CONFIG, c.value, 0) && writePowerMode(PowerMode::Normal);
    if (_periodic) {
        _latest   = 0;
        _interval = interval_table[m5::stl::to_underlying(c.standby())];
    }
    return _periodic;
}

bool UnitBMP280::stop_periodic_measurement()
{
    if (inPeriodic() && writePowerMode(PowerMode::Sleep)) {
        _periodic = false;
        return true;
    }
    return false;
}

bool UnitBMP280::measureSingleshot(bmp280::Data& d, const bmp280::Oversampling osrsPressure,
                                   const bmp280::Oversampling osrsTemperature, const bmp280::Filter filter)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (osrsTemperature == Oversampling::Skipped) {
        return false;
    }

    Config c{};
    c.filter(filter);
    CtrlMeas cm{};
    cm.osrs_p(osrsPressure);
    cm.osrs_t(osrsTemperature);
    return writeRegister8(CONFIG, c.value) && writeRegister8(CONTROL_MEASUREMENT, cm.value) && measure_singleshot(d);
}

bool UnitBMP280::measure_singleshot(bmp280::Data& d)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    if (writePowerMode(PowerMode::Forced)) {
        auto start_at   = m5::utility::millis();
        auto timeout_at = start_at + 2 * 1000;  // 2sec
        bool done{};
        do {
            PowerMode pm{};
            done = readPowerMode(pm) && (pm == PowerMode::Sleep) && is_data_ready();
            if (done) {
                break;
            }
            m5::utility::delay(1);
        } while (!done && m5::utility::millis() <= timeout_at);
        return done && read_measurement(d);
    }
    return false;
}

bool UnitBMP280::readOversampling(Oversampling& osrsPressure, Oversampling& osrsTemperature)
{
    CtrlMeas cm{};
    if (readRegister8(CONTROL_MEASUREMENT, cm.value, 0)) {
        osrsPressure    = cm.osrs_p();
        osrsTemperature = cm.osrs_t();
        return true;
    }
    return false;
}

bool UnitBMP280::writeOversampling(const Oversampling osrsPressure, const Oversampling osrsTemperature)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    CtrlMeas cm{};
    if (readRegister8(CONTROL_MEASUREMENT, cm.value, 0)) {
        cm.osrs_p(osrsPressure);
        cm.osrs_t(osrsTemperature);
        return writeRegister8(CONTROL_MEASUREMENT, cm.value);
    }
    return false;
}

bool UnitBMP280::writeOversamplingPressure(const Oversampling osrsPressure)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    CtrlMeas cm{};
    if (readRegister8(CONTROL_MEASUREMENT, cm.value, 0)) {
        cm.osrs_p(osrsPressure);
        return writeRegister8(CONTROL_MEASUREMENT, cm.value);
    }
    return false;
}

bool UnitBMP280::writeOversamplingTemperature(const Oversampling osrsTemperature)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    CtrlMeas cm{};
    if (readRegister8(CONTROL_MEASUREMENT, cm.value, 0)) {
        cm.osrs_t(osrsTemperature);
        return writeRegister8(CONTROL_MEASUREMENT, cm.value);
    }
    return false;
}

bool UnitBMP280::writeOversampling(const bmp280::OversamplingSetting osrss)
{
    auto idx = m5::stl::to_underlying(osrss);
    return writeOversampling(osrss_table[idx][0], osrss_table[idx][1]);
}

bool UnitBMP280::readPowerMode(PowerMode& m)
{
    CtrlMeas cm{};
    if (readRegister8(CONTROL_MEASUREMENT, cm.value, 0)) {
        m = cm.mode();
        return true;
    }
    return false;
}

bool UnitBMP280::writePowerMode(const PowerMode m)
{
    CtrlMeas cm{};
    if (readRegister8(CONTROL_MEASUREMENT, cm.value, 0)) {
        cm.mode(m);

        // Datasheet says
        // If the device is currently performing ameasurement,
        // execution of mode switching commands is delayed until the end of the currentlyrunning measurement period
        bool can{};
        auto timeout_at = m5::utility::millis() + 1000;
        do {
            can = is_data_ready();
            if (can) {
                break;
            }
            m5::utility::delay(1);
        } while (!can && m5::utility::millis() <= timeout_at);

        return can && writeRegister8(CONTROL_MEASUREMENT, cm.value);
    }
    return false;
}

bool UnitBMP280::readFilter(Filter& f)
{
    Config c{};
    if (readRegister8(CONFIG, c.value, 0)) {
        f = c.filter();
        return true;
    }
    return false;
}

bool UnitBMP280::writeFilter(const Filter& f)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    PowerMode pm{};
    if (!readPowerMode(pm) || pm != PowerMode::Sleep) {
        // Datasheet says
        // Writes to the config register in normal mode may be ignored. In sleep mode writes are not ignored
        M5_LIB_LOGE("Invalid power mode %02X", pm);
        return false;
    }

    Config c{};
    if (readRegister8(CONFIG, c.value, 0)) {
        c.filter(f);
        return writeRegister8(CONFIG, c.value);
    }
    return false;
}

bool UnitBMP280::readStandbyTime(Standby& s)
{
    Config c{};
    if (readRegister8(CONFIG, c.value, 0)) {
        s = c.standby();
        return true;
    }
    return false;
}

bool UnitBMP280::writeStandbyTime(const Standby s)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    Config c{};
    if (readRegister8(CONFIG, c.value, 0)) {
        c.standby(s);
        return writeRegister8(CONFIG, c.value);
    }
    return false;
}

bool UnitBMP280::writeUseCaseSetting(const bmp280::UseCase uc)
{
    const auto& tbl = uc_table[m5::stl::to_underlying(uc)];
    return writeOversampling(tbl.osrss) && writeFilter(tbl.filter) && writeStandbyTime(tbl.st);
}

bool UnitBMP280::softReset()
{
    if (writeRegister8(SOFT_RESET, RESET_VALUE)) {
        auto timeout_at = m5::utility::millis() + 100;  // 100ms
        uint8_t s{0xFF};
        do {
            if (readRegister8(GET_STATUS, s, 0) && (s & 0x01 /* im update */) == 0x00) {
                _periodic = false;
                return true;
            }
        } while ((s & 0x01) && m5::utility::millis() < timeout_at);
        return false;
    }
    return false;
}

//
bool UnitBMP280::read_trimming(Trimming& t)
{
    return readRegister(TRIMMING_DIG, t.value, m5::stl::size(t.value), 0);
}

bool UnitBMP280::is_data_ready()
{
    uint8_t s{0xFF};
    return readRegister8(GET_STATUS, s, 0) && ((s & 0x09 /* Measuring, im update */) == 0x00);
}

bool UnitBMP280::read_measurement(bmp280::Data& d)
{
    d.trimming = nullptr;

    // Datasheet says
    // Shadowing will only work if all data registers are read in a single burst read.
    // Therefore, the user must use burst reads if he does not synchronize data readout with themeasurement cycle
    if (readRegister(GET_MEASUREMENT, d.raw.data(), d.raw.size(), 0)) {
        d.trimming = &_trimming;
        return true;
    }
    return false;
}

}  // namespace unit
}  // namespace m5
