/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_QMP6988.cpp
  @brief QMP6988 Unit for M5UnitUnified
*/
#include "unit_QMP6988.hpp"
#include <M5Utility.hpp>
#include <limits>  // NaN
#include <cmath>
#include <thread>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::qmp6988;
using namespace m5::unit::qmp6988::command;

namespace {
constexpr uint8_t chip_id{0x5C};
constexpr size_t calibration_length{25};
constexpr uint32_t sub_raw{8388608};  // 2^23

constexpr PowerMode mode_table[] = {
    PowerMode::Sleep,
    PowerMode::Force,  // 0b01 and 0b10 are Force
    PowerMode::Force,  // 0b01 and 0b10 are Force
    PowerMode::Normal,
};

constexpr elapsed_time_t standby_time_table[] = {
    5, 5, 50, 250, 500, 1000, 2000, 4000,
};

constexpr float ostb{4.4933f};
constexpr float oversampling_temp_time_table[] = {
    0.0f, ostb * 1, ostb * 2, ostb * 4, ostb * 8, ostb * 16, ostb * 32, ostb * 64,
};

constexpr float ospb{0.5032f};
constexpr float oversampling_pressure_time_table[] = {
    0.0f, ospb * 1, ospb * 2, ospb * 4, ospb * 8, ospb * 16, ospb * 32, ospb * 64,
};

constexpr float filter_time_table[] = {
    0.0f, 0.3f, 0.6f, 1.2f, 2.4f, 4.8f, 9.6f, 9.6f, 9.6f,
};

int16_t convert_temperature256(const int32_t dt, const m5::unit::qmp6988::Calibration& c)
{
    int64_t wk1, wk2;
    int16_t temp256{};
    // wk1: 60Q4 // bit size
    wk1     = ((int64_t)c.a1 * (int64_t)dt);        // 31Q23+24-1=54 (54Q23)
    wk2     = ((int64_t)c.a2 * (int64_t)dt) >> 14;  // 30Q47+24-1=53 (39Q33)
    wk2     = (wk2 * (int64_t)dt) >> 10;            // 39Q33+24-1=62 (52Q23)
    wk2     = ((wk1 + wk2) / 32767) >> 19;          // 54,52->55Q23 (20Q04)
    temp256 = (int16_t)((c.a0 + wk2) >> 4);         // 21Q4 -> 17Q0
    return temp256;
}

int32_t convert_pressure16(const int32_t dp, const int16_t tx, const Calibration& c)
{
    int64_t wk1, wk2, wk3;

    // wk1 = 48Q16 // bit size
    wk1 = ((int64_t)c.bt1 * (int64_t)tx);        // 28Q15+16-1=43 (43Q15)
    wk2 = ((int64_t)c.bp1 * (int64_t)dp) >> 5;   // 31Q20+24-1=54 (49Q15)
    wk1 += wk2;                                  // 43,49->50Q15
    wk2 = ((int64_t)c.bt2 * (int64_t)tx) >> 1;   // 34Q38+16-1=49 (48Q37)
    wk2 = (wk2 * (int64_t)tx) >> 8;              // 48Q37+16-1=63 (55Q29)
    wk3 = wk2;                                   // 55Q29
    wk2 = ((int64_t)c.b11 * (int64_t)tx) >> 4;   // 28Q34+16-1=43 (39Q30)
    wk2 = (wk2 * (int64_t)dp) >> 1;              // 39Q30+24-1=62 (61Q29)
    wk3 += wk2;                                  // 55,61->62Q29
    wk2 = ((int64_t)c.bp2 * (int64_t)dp) >> 13;  // 29Q43+24-1=52 (39Q30)
    wk2 = (wk2 * (int64_t)dp) >> 1;              // 39Q30+24-1=62 (61Q29)
    wk3 += wk2;                                  // 62,61->63Q29
    wk1 += wk3 >> 14;                            // Q29 >> 14 -> Q15
    wk2 = ((int64_t)c.b12 * (int64_t)tx);        // 29Q53+16-1=45 (45Q53)
    wk2 = (wk2 * (int64_t)tx) >> 22;             // 45Q53+16-1=61 (39Q31)
    wk2 = (wk2 * (int64_t)dp) >> 1;              // 39Q31+24-1=62 (61Q30)
    wk3 = wk2;                                   // 61Q30
    wk2 = ((int64_t)c.b21 * (int64_t)tx) >> 6;   // 29Q60+16-1=45 (39Q54)
    wk2 = (wk2 * (int64_t)dp) >> 23;             // 39Q54+24-1=62 (39Q31)
    wk2 = (wk2 * (int64_t)dp) >> 1;              // 39Q31+24-1=62 (61Q20)
    wk3 += wk2;                                  // 61,61->62Q30
    wk2 = ((int64_t)c.bp3 * (int64_t)dp) >> 12;  // 28Q65+24-1=51 (39Q53)
    wk2 = (wk2 * (int64_t)dp) >> 23;             // 39Q53+24-1=62 (39Q30)
    wk2 = (wk2 * (int64_t)dp);                   // 39Q30+24-1=62 (62Q30)
    wk3 += wk2;                                  // 62,62->63Q30
    wk1 += wk3 >> 15;                            // Q30 >> 15 = Q15
    wk1 /= 32767L;
    wk1 >>= 11;    // Q15 >> 7 = Q4
    wk1 += c.b00;  // Q4 + 20Q4
    // Not shifted to set output at 16 Pa
    // wk1 >>= 4;     // 28Q4 -> 24Q0
    int32_t p16 = (int32_t)wk1;
    return p16;
}

}  // namespace

namespace m5 {
namespace unit {

namespace qmp6988 {
float Data::celsius() const
{
    if (calib) {
        uint32_t rt  = (((uint32_t)raw[3]) << 16) | (((uint32_t)raw[4]) << 8) | ((uint32_t)raw[5]);
        int32_t dt   = (int32_t)(rt - sub_raw);
        int16_t t256 = convert_temperature256(dt, *calib);
        return (float)t256 / 256.f;
    }
    return std::numeric_limits<float>::quiet_NaN();
}

float Data::fahrenheit() const
{
    return celsius() * 9.0f / 5.0f + 32.f;
}

float Data::pressure() const
{
    if (calib) {
        uint32_t rt  = (((uint32_t)raw[3]) << 16) | (((uint32_t)raw[4]) << 8) | ((uint32_t)raw[5]);
        int32_t dt   = (int32_t)(rt - sub_raw);
        int16_t t256 = convert_temperature256(dt, *calib);

        uint32_t rp = (((uint32_t)raw[0]) << 16) | (((uint32_t)raw[1]) << 8) | ((uint32_t)raw[2]);
        int32_t dp  = (int32_t)(rp - sub_raw);
        int32_t p16 = convert_pressure16(dp, t256, *calib);
        return (float)p16 / 16.0f;
    }
    return std::numeric_limits<float>::quiet_NaN();
}
};  // namespace qmp6988

//
const char UnitQMP6988::name[] = "UnitQMP6988";
const types::uid_t UnitQMP6988::uid{"UnitQMP6988"_mmh3};
const types::uid_t UnitQMP6988::attr{0};

types::elapsed_time_t UnitQMP6988::calculatInterval(const Standby st, const Oversampling ost, const Oversampling osp,
                                                    const Filter f)
{
    // M5_LIB_LOGV("ST:%u OST:%u OSP:%u F:%u", st, ost, osp, f);
    // M5_LIB_LOGV(
    //     "Value ST:%u OST:%u OSP:%u F:%u",
    // standby_time_table[m5::stl::to_underlying(st)],
    // (elapsed_time_t)std::ceil(
    //     oversampling_temp_time_table[m5::stl::to_underlying(ost)]),
    // (elapsed_time_t)std::ceil(
    //     oversampling_pressure_time_table[m5::stl::to_underlying(osp)]),
    // (elapsed_time_t)std::ceil(
    //     filter_time_table[m5::stl::to_underlying(f)]));

    elapsed_time_t itv = standby_time_table[m5::stl::to_underlying(st)] +
                         (elapsed_time_t)std::ceil(oversampling_temp_time_table[m5::stl::to_underlying(ost)]) +
                         (elapsed_time_t)std::ceil(oversampling_pressure_time_table[m5::stl::to_underlying(osp)]) +
                         (elapsed_time_t)std::ceil(filter_time_table[m5::stl::to_underlying(f)]);
    return itv;
}

bool UnitQMP6988::begin()
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
    if (!readRegister8(CHIP_ID, id, 0) || id != chip_id) {
        M5_LIB_LOGE("This unit is NOT QMP6988 %x", id);
        return false;
    }

    if (!reset()) {
        M5_LIB_LOGE("Failed to reset");
        return false;
    }

    if (!read_calibration(_calibration)) {
        M5_LIB_LOGE("Failed to read_calibration");
        return false;
    }

    if (!writeOversamplings(_cfg.oversampling_temperature, _cfg.oversampling_pressure) ||
        !writeFilterCoeff(_cfg.filter) || !writeStandbyTime(_cfg.standby_time)) {
        M5_LIB_LOGE("Failed to settings");
        return false;
    }
    return _cfg.start_periodic ? startPeriodicMeasurement() : writePowerMode(qmp6988::PowerMode::Sleep);
}

void UnitQMP6988::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = (wait_measurement(_interval) && read_measurement(d));
            if (_updated) {
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool UnitQMP6988::start_periodic_measurement()
{
    if (inPeriodic()) {
        return false;
    }
    return writePowerMode(qmp6988::PowerMode::Normal);
}

bool UnitQMP6988::start_periodic_measurement(const qmp6988::Standby st, const qmp6988::Oversampling ost,
                                             const qmp6988::Oversampling osp, const qmp6988::Filter& f)
{
    if (inPeriodic()) {
        return false;
    }

    CtrlMeasurement cm{};
    cm.oversamplingTemperature(ost);
    cm.oversamplingPressure(osp);
    cm.mode(PowerMode::Normal);
    return writePowerMode(PowerMode::Sleep) && writeFilterCoeff(f) && writeStandbyTime(st) &&
           write_measurement_condition(cm.value);
}

bool UnitQMP6988::stop_periodic_measurement()
{
    return writePowerMode(qmp6988::PowerMode::Sleep);
}

bool UnitQMP6988::measureSingleshot(qmp6988::Data& d)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    return writePowerMode(qmp6988::PowerMode::Force) && wait_measurement() && read_measurement(d);
}

bool UnitQMP6988::measureSingleshot(qmp6988::Data& d, const qmp6988::Oversampling ost, const qmp6988::Oversampling osp,
                                    const qmp6988::Filter& f)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    CtrlMeasurement cm{};
    cm.oversamplingTemperature(ost);
    cm.oversamplingPressure(osp);
    cm.mode(PowerMode::Force);
    return writePowerMode(PowerMode::Sleep) && writeFilterCoeff(f) && write_measurement_condition(cm.value) &&
           wait_measurement() && read_measurement(d);
}

bool UnitQMP6988::readOversamplings(qmp6988::Oversampling& ost, qmp6988::Oversampling& osp)
{
    qmp6988::CtrlMeasurement cm{};
    if (read_measurement_condition(cm.value)) {
        ost = cm.oversamplingTemperature();
        osp = cm.oversamplingPressure();
        return true;
    }
    return false;
}

bool UnitQMP6988::writeOversamplings(const qmp6988::Oversampling ost, const qmp6988::Oversampling osp)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    qmp6988::CtrlMeasurement cm{};
    if (read_measurement_condition(cm.value)) {
        cm.oversamplingTemperature(ost);
        cm.oversamplingPressure(osp);
        return write_measurement_condition(cm.value);
    }
    return false;
}

bool UnitQMP6988::writeOversamplingTemperature(const qmp6988::Oversampling os)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    qmp6988::CtrlMeasurement cm{};
    if (read_measurement_condition(cm.value)) {
        cm.oversamplingTemperature(os);
        return write_measurement_condition(cm.value);
    }
    return false;
}

bool UnitQMP6988::writeOversamplingPressure(const qmp6988::Oversampling os)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    qmp6988::CtrlMeasurement cm{};
    if (read_measurement_condition(cm.value)) {
        cm.oversamplingPressure(os);
        return write_measurement_condition(cm.value);
    }
    return false;
}

bool UnitQMP6988::readPowerMode(qmp6988::PowerMode& m)
{
    qmp6988::CtrlMeasurement cm{};
    if (read_measurement_condition(cm.value)) {
        m = mode_table[m5::stl::to_underlying(cm.mode())];
        return true;
    }
    return false;
}

bool UnitQMP6988::writePowerMode(const qmp6988::PowerMode m)
{
    qmp6988::CtrlMeasurement cm{};
    if (read_measurement_condition(cm.value)) {
        cm.mode(m);
        return write_measurement_condition(cm.value);
    }
    return false;
}

bool UnitQMP6988::readFilterCoeff(qmp6988::Filter& f)
{
    constexpr qmp6988::Filter table[] = {
        qmp6988::Filter::Off,
        qmp6988::Filter::Coeff2,
        qmp6988::Filter::Coeff4,
        qmp6988::Filter::Coeff8,
        qmp6988::Filter::Coeff16,
        // 0b101, 0b110, 0b111 are 32
        qmp6988::Filter::Coeff32,
        qmp6988::Filter::Coeff32,
        qmp6988::Filter::Coeff32,
    };

    uint8_t v{};
    if (readRegister8(IIR_FILTER, v, 0)) {
        f = table[v & 0x07];
        return true;
    }
    return false;
}

bool UnitQMP6988::writeFilterCoeff(const qmp6988::Filter& f)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    if (writeRegister8(IIR_FILTER, m5::stl::to_underlying(f))) {
        _filter = f;
        return true;
    }
    return false;
}

bool UnitQMP6988::readStandbyTime(qmp6988::Standby& st)
{
    qmp6988::IOSetup is{};
    if (read_io_setup(is.value)) {
        st = is.standby();
        return true;
    }
    return false;
}

bool UnitQMP6988::writeStandbyTime(const qmp6988::Standby st)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }

    qmp6988::IOSetup is{};
    if (read_io_setup(is.value)) {
        if (st == is.standby()) {
            return true;
        }
        is.standby(st);
        if (write_io_setup(is.value)) {
            _standby = st;
            return true;
        }
    }
    return false;
}

bool UnitQMP6988::reset()
{
    uint8_t v{0xE6};  // When inputting "E6h", a soft-reset will be occurred

    auto ret = writeRegister8(RESET, v);
    M5_LIB_LOGD("Reset causes a NO ACK or timeout error, but ignore it");
    (void)ret;
    // TODO / WARNING (HAL)
    m5::utility::delay(10);             // Need delay
    return writeRegister(RESET, 0x00);  // Nothing to happen
}

bool UnitQMP6988::readStatus(qmp6988::Status& s)
{
    return readRegister8(GET_STATUS, s.value, 1);
}

bool UnitQMP6988::read_measurement_condition(uint8_t& cond)
{
    return readRegister8(CONTROL_MEASUREMENT, cond, 0);
}

bool UnitQMP6988::write_measurement_condition(const uint8_t cond)
{
    if (writeRegister8(CONTROL_MEASUREMENT, cond)) {
        auto prev = _mode;
        qmp6988::CtrlMeasurement cm;
        cm.value    = cond;
        _osTemp     = cm.oversamplingTemperature();
        _osPressure = cm.oversamplingPressure();
        _mode       = cm.mode();
        // Power mode state changes affect internal operation
        _periodic = (_mode == qmp6988::PowerMode::Normal);
        if (_periodic && _mode != prev) {
            _latest   = 0;
            _interval = calculatInterval(_standby, _osTemp, _osPressure, _filter);
            // M5_LIB_LOGE("Interval:%lu", _interval);
        }
        return true;
    }
    return false;
}

bool UnitQMP6988::read_io_setup(uint8_t& s)
{
    return readRegister8(IO_SETUP, s, 0);
}

bool UnitQMP6988::write_io_setup(const uint8_t s)
{
    return writeRegister8(IO_SETUP, s);
}

// Wait until measured
bool UnitQMP6988::wait_measurement(const uint32_t timeout)
{
    if (_mode != qmp6988::PowerMode::Sleep) {
        auto timeout_at = m5::utility::millis() + timeout;
        do {
            if (is_ready_data()) {
                return true;
            }
            std::this_thread::yield();
            // m5::utility::delay(1);
        } while (m5::utility::millis() <= timeout_at);
    }
    return false;
}

bool UnitQMP6988::is_ready_data()
{
    Status s{};
    return readStatus(s) && !s.measure();
}

bool UnitQMP6988::read_measurement(Data& d)
{
    if (_mode == qmp6988::PowerMode::Sleep) {
        M5_LIB_LOGW("Sleeping");
        return false;
    }
    if (_osTemp == qmp6988::Oversampling::Skip && _osPressure == qmp6988::Oversampling::Skip) {
        M5_LIB_LOGW("Cannot be measured");
        return false;
    }
    if (readRegister(READ_PRESSURE, d.raw.data(), d.raw.size(), 0)) {
        d.calib = &_calibration;
        return true;
    }
    return false;
}

bool UnitQMP6988::read_calibration(qmp6988::Calibration& c)
{
    using namespace m5::utility;  // unsigned_to_signed
    using namespace m5::types;    // big_uint16_t

    uint8_t rbuf[calibration_length]{};
    if (!readRegister(READ_COMPENSATION_COEFFICIENT, rbuf, sizeof(rbuf), 0)) {
        return false;
    }

    uint32_t b00 = ((uint32_t)(big_uint16_t(rbuf[0], rbuf[1]).get()) << 4) | ((rbuf[24] >> 4) & 0x0F);
    c.b00        = unsigned_to_signed<20>(b00);                                                                 // 20Q4
    c.bt1        = 2982L * (int64_t)unsigned_to_signed<16>(big_uint16_t(rbuf[2], rbuf[3]).get()) + 107370906L;  // 28Q15
    c.bt2 = 329854L * (int64_t)unsigned_to_signed<16>(big_uint16_t(rbuf[4], rbuf[5]).get()) + +108083093L;      // 34Q38
    c.bp1 = 19923L * (int64_t)unsigned_to_signed<16>(big_uint16_t(rbuf[6], rbuf[7]).get()) + 1133836764L;       // 31Q20
    c.b11 = 2406L * (int64_t)unsigned_to_signed<16>(big_uint16_t(rbuf[8], rbuf[9]).get()) + 118215883L;         // 28Q34
    c.bp2 = 3079L * (int64_t)unsigned_to_signed<16>(big_uint16_t(rbuf[10], rbuf[11]).get()) - 181579595L;       // 29Q43
    c.b12 = 6846L * (int64_t)unsigned_to_signed<16>(big_uint16_t(rbuf[12], rbuf[13]).get()) + 85590281L;        // 29Q53
    c.b21 = 13836L * (int64_t)unsigned_to_signed<16>(big_uint16_t(rbuf[14], rbuf[15]).get()) + 79333336L;       // 29Q60
    c.bp3 = 2915L * (int64_t)unsigned_to_signed<16>(big_uint16_t(rbuf[16], rbuf[17]).get()) + 157155561L;       // 28Q65
    uint32_t a0 = ((uint32_t)big_uint16_t(rbuf[18], rbuf[19]).get() << 4) | (rbuf[24] & 0x0F);
    c.a0        = unsigned_to_signed<20>(a0);                                                              // 20Q4
    c.a1 = 3608L * (int32_t)unsigned_to_signed<16>(big_uint16_t(rbuf[20], rbuf[21]).get()) - 1731677965L;  // 31Q23
    c.a2 = 16889L * (int32_t)unsigned_to_signed<16>(big_uint16_t(rbuf[22], rbuf[23]).get()) - 87619360L;   // 31Q47
#if 0
    M5_LIB_LOGI(
        "\n"
        "b00:%d\n"
        "bt1:%d\n"
        "bt2:%lld\n"
        "bp1:%d\n"
        "b11:%d\n"
        "bp2:%d\n"
        "b12:%d\n"
        "b21:%d\n"
        "bp3:%d\n"
        "a0:%d\n"
        "a1:%d\n"
        "a2:%d",
        c.b00, c.bt1, c.bt2, c.bp1, c.b11, c.bp2, c.b12, c.b21, c.bp3, c.a0,
        c.a1, c.a2);
#endif
    return true;
}
}  // namespace unit
}  // namespace m5
