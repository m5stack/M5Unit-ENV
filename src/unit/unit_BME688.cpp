/*!
  @file unit_BME688.cpp
  @brief BME688 Unit for M5UnitUnified

  SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD

  SPDX-License-Identifier: MIT


*/
#include "unit_BME688.hpp"
#if defined(UNIT_BME688_USING_BSEC2)
#include <inc/bsec_interface.h>  // BSEC2
#endif
#include <M5Utility.hpp>
#include <array>
#include <cmath>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::bme688;
using namespace m5::unit::bme688::command;

static_assert(std::is_same<std::underlying_type<Oversampling>::type,
                           decltype(bme68xConf::os_temp)>::value,
              "Illegal type");
static_assert(std::is_same<std::underlying_type<Oversampling>::type,
                           decltype(bme68xConf::os_pres)>::value,
              "Illegal type");
static_assert(std::is_same<std::underlying_type<Oversampling>::type,
                           decltype(bme68xConf::os_hum)>::value,
              "Illegal type");
static_assert(std::is_same<std::underlying_type<Filter>::type,
                           decltype(bme68xConf::filter)>::value,
              "Illegal type");
static_assert(std::is_same<std::underlying_type<Mode>::type, uint8_t>::value,
              "Illegal type");

namespace {
constexpr Oversampling oversampling_table[8] = {
    Oversampling::None, Oversampling::x1,  Oversampling::x2,  Oversampling::x4,
    Oversampling::x8,   Oversampling::x16, Oversampling::x16, Oversampling::x16,

};

#if defined(UNIT_BME688_USING_BSEC2)
constexpr float sample_rate_table[] = {
    BSEC_SAMPLE_RATE_DISABLED, BSEC_SAMPLE_RATE_LP,
    BSEC_SAMPLE_RATE_ULP,      BSEC_SAMPLE_RATE_ULP_MEASUREMENT_ON_DEMAND,
    BSEC_SAMPLE_RATE_SCAN,     BSEC_SAMPLE_RATE_CONT,
};
#endif

constexpr uint8_t VALID_DATA{0xB0};

void delay_us_function(uint32_t period, void* /*intf_ptr*/) {
    m5::utility::delayMicroseconds(period);
}

bool operator==(const Mode m, const uint8_t o) {
    assert(o <= m5::stl::to_underlying(Mode::Sequential) && "Illegal value");
    return m5::stl::to_underlying(m) == o;
}

bool operator==(const uint8_t o, const Mode m) {
    assert(o <= m5::stl::to_underlying(Mode::Sequential) && "Illegal value");
    return m5::stl::to_underlying(m) == o;
}

inline bool operator!=(const Mode m, const uint8_t o) {
    return !(m == o);
}

inline bool operator!=(const uint8_t o, const Mode m) {
    return !(o == m);
}

}  // namespace

namespace m5 {
namespace unit {
//
const char UnitBME688::name[] = "UnitBME688";
const types::uid_t UnitBME688::uid{"UnitBME688"_mmh3};
const types::uid_t UnitBME688::attr{0};

// I2C accessor
int8_t UnitBME688::read_function(uint8_t reg_addr, uint8_t* reg_data,
                                 uint32_t length, void* intf_ptr) {
    assert(intf_ptr);
    UnitBME688* unit = (UnitBME688*)intf_ptr;
    return unit->readRegister(reg_addr, reg_data, length, 0)
               ? BME68X_OK
               : BME68X_E_COM_FAIL;
}

int8_t UnitBME688::write_function(uint8_t reg_addr, const uint8_t* reg_data,
                                  uint32_t length, void* intf_ptr) {
    assert(intf_ptr);
    UnitBME688* unit = (UnitBME688*)intf_ptr;
    return unit->writeRegister(reg_addr, reg_data, length) ? BME68X_OK
                                                           : BME68X_E_COM_FAIL;
}

UnitBME688::UnitBME688(const uint8_t addr)
    : Component(addr)
#if defined(UNIT_BME688_USING_BSEC2)
      ,
      _bsec2_work{new uint8_t[BSEC_MAX_PROPERTY_BLOB_SIZE]}
#endif
{
    _dev.intf     = BME68X_I2C_INTF;
    _dev.read     = UnitBME688::read_function;
    _dev.write    = UnitBME688::write_function;
    _dev.delay_us = delay_us_function;
    _dev.intf_ptr = this;
    _dev.amb_temp = 25;
#if defined(UNIT_BME688_USING_BSEC2)
    assert(_bsec2_work);
#endif
}

bool UnitBME688::begin() {
    if (bme68x_init(&_dev) != BME68X_OK) {
        M5_LIB_LOGE("Failed to initialize");
        return false;
    }

#if defined(UNIT_BME688_USING_BSEC2)
    auto ret = bsec_init();
    if (ret != BSEC_OK) {
        M5_LIB_LOGE("Failed to bsec_init %d", ret);
        return false;
    }
#endif

#if 0    
    if (bme68x_get_conf(&_tphConf, &_dev) != BME68X_OK ||
        bme68x_get_heatr_conf(&_heaterConf, &_dev) != BME68X_OK) {
        M5_LIB_LOGE("Failed to read settings");
        return false;
    }

    M5_LIB_LOGV(
        "Calibration\n"
        "T:%u/%d/%d\n"
        "P:%u/%d/%d/%d/%d/%d/%d/%d/%d/%u\n"
        "H:%u/%u/%d/%d/%d/%u/%d\n"
        "G:%d/%d/%d/%u/%u",
        _dev.calib.par_t1, _dev.calib.par_t2, _dev.calib.par_t3,
        _dev.calib.par_p1, _dev.calib.par_p2, _dev.calib.par_p3,
        _dev.calib.par_p4, _dev.calib.par_p5, _dev.calib.par_p6,
        _dev.calib.par_p7, _dev.calib.par_p8, _dev.calib.par_p9,
        _dev.calib.par_p10, _dev.calib.par_h1, _dev.calib.par_h2,
        _dev.calib.par_h3, _dev.calib.par_h4, _dev.calib.par_h5,
        _dev.calib.par_h6, _dev.calib.par_h7, _dev.calib.par_gh1,
        _dev.calib.par_gh2, _dev.calib.par_gh3, _dev.calib.res_heat_range,
        _dev.calib.res_heat_val);

    M5_LIB_LOGV("TPH: T:%u P:%u H:%u F:%u O:%u", _tphConf.os_temp, _tphConf.os_pres,
                _tphConf.os_hum, _tphConf.filter, _tphConf.odr);
    M5_LIB_LOGV(
        "Heater: enable:%u\n"
        "Forced: T:%u D:%u\n"
        "Parallel: len:%u shared:%u",
        _heaterConf.enable, _heaterConf.heatr_temp, _heaterConf.heatr_dur,
        _heaterConf.profile_len, _heaterConf.shared_heatr_dur);
    M5_DUMPV(_heaterConf.heatr_temp_prof, _heaterConf.profile_len * 2);
    M5_DUMPV(_heaterConf.heatr_dur_prof, _heaterConf.profile_len * 2);
#endif

#if defined(UNIT_BME688_USING_BSEC2)
    return bsec_get_version(&_bsec2_version) == BSEC_OK;
#else
    return true;
#endif
}

void UnitBME688::update(const bool force) {
    _updated = false;
#if defined(UNIT_BME688_USING_BSEC2)
    if (_bsec2_subscription) {
        update_bsec2(force);
    } else {
        update_bme688(force);
    }
#else
    update_bme688(force);
#endif
}

#if defined(UNIT_BME688_USING_BSEC2)
// Using BSEC2 library and configration and state
void UnitBME688::update_bsec2(const bool force) {
    auto now       = m5::utility::millis();
    int64_t now_ns = now * 1000000ULL;  // ms to ns

    _bsec2_mode = static_cast<Mode>(_bsec2_settings.op_mode);

    if (!force && now_ns < _bsec2_settings.next_call) {
        return;
    }

    // M5_LIB_LOGW("_bsec2_mode:%u", _bsec2_mode);

    auto ret = bsec_sensor_control(now_ns, &_bsec2_settings);
    if (ret != BSEC_OK) {
        M5_LIB_LOGW("Failed to bsec_sensor_control %d", ret);
        return;
    }

    // M5_LIB_LOGW("_bsec2_settings.op_mode:%u", _bsec2_settings.op_mode);

    switch (_bsec2_settings.op_mode) {
        case BME68X_FORCED_MODE:
            if (!set_forced_mode()) {
                return;
            }
            _bsec2_mode = Mode::Forced;
            break;
        case BME68X_SLEEP_MODE:
            if (_bsec2_mode != _bsec2_settings.op_mode &&
                setMode(Mode::Sleep)) {
                _bsec2_mode = Mode::Sleep;
            }
            break;
        case BME68X_PARALLEL_MODE:
            if (_bsec2_mode != _bsec2_settings.op_mode) {
                if (!set_parallel_mode()) {
                    return;
                }
                _bsec2_mode = Mode::Parallel;
            }
            break;
        default:
            return;
    }

    if (_bsec2_settings.trigger_measurement &&
        _bsec2_settings.op_mode != BME68X_SLEEP_MODE) {
        if (fetch_data() && _num_of_data) {
            uint32_t idx{0};
            do {
                auto& d = _data[idx];
                if (d.status & BME68X_GASM_VALID_MSK) {
                    d.pressure *= 0.01f;  // Conversion from Pa to hPa
                    if (!process_data(now_ns, d)) {
                        M5_LIB_LOGE("Failed to process_data");
                        return;
                    }
                }
            } while (++idx < _num_of_data);
            _updated = true;
            _latest  = now;
        }
    }
}
#endif

// Directly use  BME688 (but only raw data can be obtained)
void UnitBME688::update_bme688(const bool force) {
    if (!inPeriodic()) {
        return;
    }

    elapsed_time_t at{m5::utility::millis()};
    if (force || !_latest || at >= _latest + _interval) {
        _updated = read_measurement();
        if (_updated) {
            switch (_mode) {
                    // Forced goes to sleep after measurement, so set it up
                    // again
                case Mode::Forced:
                    if (!setMode(Mode::Forced)) {
                        M5_LIB_LOGE("Failed to sete mode again");
                        _updated  = false;
                        _mode     = Mode::Sleep;
                        _periodic = false;
                        return;
                    }
                    break;
                case Mode::Parallel:
                    if (!_num_of_data || _data[0].status != VALID_DATA) {
                        _updated = false;
                        return;
                    }
                    break;
                case Mode::Sequential:
                    if (!_num_of_data) {
                        _updated = false;
                        return;
                    }
                    break;
                default:
                    _updated = false;
                    return;
            }
            _latest = at;
        }
    }
}

bool UnitBME688::readUniqueID(uint32_t& id) {
    // Order 2-3-1-0
    // See also
    // https://community.bosch-sensortec.com/t5/MEMS-sensors-forum/Unique-IDs-in-Bosch-Sensors/td-p/6012
    std::array<uint8_t, 4> rbuf{};
    if (readRegister(UNIQUE_ID, rbuf.data(), rbuf.size(), 0)) {
        uint32_t id1 = ((uint32_t)rbuf[3] + ((uint32_t)rbuf[2] << 8)) & 0x7fff;
        id = (id1 << 16) + (((uint32_t)rbuf[1]) << 8) + (uint32_t)rbuf[0];
        return true;
    }
    return false;
}

bool UnitBME688::softReset() {
    return (bme68x_soft_reset(&_dev) == BME68X_OK) &&
           // reload settings
           bme68x_get_conf(&_tphConf, &_dev) == BME68X_OK &&
           bme68x_get_heatr_conf(&_heaterConf, &_dev) == BME68X_OK;
}

bool UnitBME688::selfTest() {
    return bme68x_selftest_check(&_dev) == BME68X_OK;
}

bool UnitBME688::readCalibration(bme688::Calibration& c) {
    std::array<uint8_t, 23> array0{};  // 0x8A
    std::array<uint8_t, 14> array1{};  // 0xE1
    std::array<uint8_t, 3> array2{};   // 0x00

    if (!readRegister(CALIBRATION_GROUP_0, array0.data(), array0.size(), 0) ||
        !readRegister(CALIBRATION_GROUP_1, array1.data(), array1.size(), 0) ||
        !readRegister(CALIBRATION_GROUP_2, array2.data(), array2.size(), 0)) {
        return false;
    }

    // temperature
    c.par_t1 = *(uint16_t*)(array1.data() + (CALIBRATION_TEMPERATURE_1_LOW -
                                             CALIBRATION_GROUP_1));
    c.par_t2 = *(int16_t*)(array0.data() + (CALIBRATION_TEMPERATURE_2_LOW -
                                            CALIBRATION_GROUP_0));
    c.par_t3 = array0[CALIBRATION_TEMPERATURE_3 - CALIBRATION_GROUP_0];
    // pressure
    c.par_p1  = *(uint16_t*)(array0.data() +
                            (CALIBRATION_PRESSURE_1_LOW - CALIBRATION_GROUP_0));
    c.par_p2  = *(int16_t*)(array0.data() +
                           (CALIBRATION_PRESSURE_2_LOW - CALIBRATION_GROUP_0));
    c.par_p3  = (int8_t)array0[CALIBRATION_PRESSURE_3 - CALIBRATION_GROUP_0];
    c.par_p4  = *(int16_t*)(array0.data() +
                           (CALIBRATION_PRESSURE_4_LOW - CALIBRATION_GROUP_0));
    c.par_p5  = *(int16_t*)(array0.data() +
                           (CALIBRATION_PRESSURE_5_LOW - CALIBRATION_GROUP_0));
    c.par_p6  = (int8_t)array0[CALIBRATION_PRESSURE_6 - CALIBRATION_GROUP_0];
    c.par_p7  = (int8_t)array0[CALIBRATION_PRESSURE_7 - CALIBRATION_GROUP_0];
    c.par_p8  = *(int16_t*)(array0.data() +
                           (CALIBRATION_PRESSURE_8_LOW - CALIBRATION_GROUP_0));
    c.par_p9  = *(int16_t*)(array0.data() +
                           (CALIBRATION_PRESSURE_9_LOW - CALIBRATION_GROUP_0));
    c.par_p10 = array0[CALIBRATION_PRESSURE_10 - CALIBRATION_GROUP_0];
    // humidity
    c.par_h1 =
        (array1[CALIBRATION_HUMIDITY_12 - CALIBRATION_GROUP_1] & 0X0F) |
        (((uint16_t)array1[CALIBRATION_HUMIDITY_1_HIGH - CALIBRATION_GROUP_1])
         << 4);
    c.par_h2 =
        ((array1[CALIBRATION_HUMIDITY_12 - CALIBRATION_GROUP_1] >> 4) & 0X0F) |
        (((uint16_t)array1[CALIBRATION_HUMIDITY_2_HIGH - CALIBRATION_GROUP_1])
         << 4);
    c.par_h3 = (int8_t)array1[CALIBRATION_HUMIDITY_3 - CALIBRATION_GROUP_1];
    c.par_h4 = (int8_t)array1[CALIBRATION_HUMIDITY_4 - CALIBRATION_GROUP_1];
    c.par_h5 = (int8_t)array1[CALIBRATION_HUMIDITY_5 - CALIBRATION_GROUP_1];
    c.par_h6 = array1[CALIBRATION_HUMIDITY_6 - CALIBRATION_GROUP_1];
    c.par_h7 = (int8_t)array1[CALIBRATION_HUMIDITY_7 - CALIBRATION_GROUP_1];
    // gas
    c.par_gh1 = (int8_t)array1[CALIBRATION_GAS_1 - CALIBRATION_GROUP_1];
    c.par_gh2 = *(int16_t*)(array1.data() +
                            (CALIBRATION_GAS_2_LOW - CALIBRATION_GROUP_1));
    c.par_gh3 = (int8_t)array1[CALIBRATION_GAS_3 - CALIBRATION_GROUP_1];
    c.res_heat_range =
        (array2[CALIBRATION_RES_HEAT_RANGE - CALIBRATION_GROUP_2] >> 4) & 0x03;
    c.res_heat_val =
        (int8_t)array2[CALIBRATION_RES_HEAT_VAL - CALIBRATION_GROUP_2];

    return true;
}

bool UnitBME688::setCalibration(const bme688::Calibration& c) {
    std::array<uint8_t, 23> array0{};  // 0x8A
    std::array<uint8_t, 14> array1{};  // 0xE1
    std::array<uint8_t, 3> array2{};   // 0x00

    // Read once to retain unused values
    if (!readRegister(CALIBRATION_GROUP_0, array0.data(), array0.size(), 0) ||
        !readRegister(CALIBRATION_GROUP_1, array1.data(), array1.size(), 0) ||
        !readRegister(CALIBRATION_GROUP_2, array2.data(), array2.size(), 0)) {
        return false;
    }

    // temperature
    *(uint16_t*)(array1.data() + (CALIBRATION_TEMPERATURE_1_LOW -
                                  CALIBRATION_GROUP_1))     = c.par_t1;
    *(int16_t*)(array0.data() + (CALIBRATION_TEMPERATURE_2_LOW -
                                 CALIBRATION_GROUP_0))      = c.par_t2;
    array0[CALIBRATION_TEMPERATURE_3 - CALIBRATION_GROUP_0] = c.par_t3;
    // pressure
    *(uint16_t*)(array0.data() +
                 (CALIBRATION_PRESSURE_1_LOW - CALIBRATION_GROUP_0)) = c.par_p1;
    *(int16_t*)(array0.data() +
                (CALIBRATION_PRESSURE_2_LOW - CALIBRATION_GROUP_0))  = c.par_p2;
    array0[CALIBRATION_PRESSURE_3 - CALIBRATION_GROUP_0]             = c.par_p3;
    *(int16_t*)(array0.data() +
                (CALIBRATION_PRESSURE_4_LOW - CALIBRATION_GROUP_0))  = c.par_p4;
    *(int16_t*)(array0.data() +
                (CALIBRATION_PRESSURE_5_LOW - CALIBRATION_GROUP_0))  = c.par_p5;
    array0[CALIBRATION_PRESSURE_6 - CALIBRATION_GROUP_0]             = c.par_p6;
    array0[CALIBRATION_PRESSURE_7 - CALIBRATION_GROUP_0]             = c.par_p7;
    *(int16_t*)(array0.data() +
                (CALIBRATION_PRESSURE_8_LOW - CALIBRATION_GROUP_0))  = c.par_p8;
    *(int16_t*)(array0.data() +
                (CALIBRATION_PRESSURE_9_LOW - CALIBRATION_GROUP_0))  = c.par_p9;
    array0[CALIBRATION_PRESSURE_10 - CALIBRATION_GROUP_0] = c.par_p10;
    // humidity
    uint8_t h12{(uint8_t)((c.par_h1 & 0x0F) | ((c.par_h2 & 0x0F) << 4))};
    array1[CALIBRATION_HUMIDITY_12 - CALIBRATION_GROUP_1] = h12;
    array1[CALIBRATION_HUMIDITY_1_HIGH - CALIBRATION_GROUP_1] =
        (c.par_h1 >> 4) & 0xFF;
    array1[CALIBRATION_HUMIDITY_2_HIGH - CALIBRATION_GROUP_1] =
        (c.par_h2 >> 4) & 0xFF;
    array1[CALIBRATION_HUMIDITY_3 - CALIBRATION_GROUP_1] = c.par_h3;
    array1[CALIBRATION_HUMIDITY_4 - CALIBRATION_GROUP_1] = c.par_h4;
    array1[CALIBRATION_HUMIDITY_5 - CALIBRATION_GROUP_1] = c.par_h5;
    array1[CALIBRATION_HUMIDITY_6 - CALIBRATION_GROUP_1] = c.par_h6;
    array1[CALIBRATION_HUMIDITY_7 - CALIBRATION_GROUP_1] = c.par_h7;
    // gas
    array1[CALIBRATION_GAS_1 - CALIBRATION_GROUP_1] = c.par_gh1;
    *(int16_t*)(array1.data() + (CALIBRATION_GAS_2_LOW - CALIBRATION_GROUP_1)) =
        c.par_gh2;
    array1[CALIBRATION_GAS_3 - CALIBRATION_GROUP_1] = c.par_gh3;
    array2[CALIBRATION_RES_HEAT_RANGE - CALIBRATION_GROUP_2] &= ~(0x03 << 4);
    array2[CALIBRATION_RES_HEAT_RANGE - CALIBRATION_GROUP_2] |=
        (c.res_heat_range & 0x03) << 4;
    array2[CALIBRATION_RES_HEAT_VAL - CALIBRATION_GROUP_2] = c.res_heat_val;

    return writeRegister(CALIBRATION_GROUP_0, array0.data(), array0.size()) &&
           writeRegister(CALIBRATION_GROUP_1, array0.data(), array1.size()) &&
           writeRegister(CALIBRATION_GROUP_2, array0.data(), array2.size());
}

bool UnitBME688::readTPHSetting(bme688::bme68xConf& s) {
    return bme68x_get_conf(&s, &_dev) == BME68X_OK;
}

bool UnitBME688::setTPHSetting(const bme688::bme68xConf& s) {
    // bme68x_set_conf argument does not const...
    if (bme68x_set_conf(const_cast<struct bme68x_conf*>(&s), &_dev) ==
        BME68X_OK) {
        _tphConf = s;
        return true;
    }
    return false;
}

bool UnitBME688::readOversamplingTemperature(bme688::Oversampling& os) {
    uint8_t v{};
    if (readRegister8(CTRL_MEASUREMENT, v, 0)) {
        v  = (v >> 5) & 0x07;
        os = static_cast<Oversampling>(oversampling_table[v]);
        return true;
    }
    return false;
}

bool UnitBME688::readOversamplingPressure(bme688::Oversampling& os) {
    uint8_t v{};
    if (readRegister8(CTRL_MEASUREMENT, v, 0)) {
        v  = (v >> 2) & 0x07;
        os = static_cast<Oversampling>(oversampling_table[v]);
        return true;
    }
    return false;
}

bool UnitBME688::readOversamplingHumidity(bme688::Oversampling& os) {
    uint8_t v{};
    if (readRegister8(CTRL_HUMIDITY, v, 0)) {
        v &= 0x07;
        os = static_cast<Oversampling>(oversampling_table[v]);
        return true;
    }
    return false;
}

bool UnitBME688::readIIRFilter(bme688::Filter& f) {
    uint8_t v{};
    if (readRegister8(CONFIG, v, 0)) {
        v = (v >> 2) & 0x07;
        f = static_cast<Filter>(v);
        return true;
    }
    return false;
}

bool UnitBME688::setOversampling(const bme688::Oversampling t,
                                 const bme688::Oversampling p,
                                 const bme688::Oversampling h) {
    uint8_t tp{}, hm{};
    if (readRegister8(CTRL_MEASUREMENT, tp, 0) &&
        readRegister8(CTRL_HUMIDITY, hm, 0)) {
        tp = (tp & ~((0x07 << 5) | (0x07 << 2))) |
             (m5::stl::to_underlying(t) << 5) |
             (m5::stl::to_underlying(p) << 2);
        hm = (hm & ~0x07) | m5::stl::to_underlying(h);
        if (writeRegister8(CTRL_MEASUREMENT, tp) &&
            writeRegister8(CTRL_HUMIDITY, hm)) {
            _tphConf.os_temp = m5::stl::to_underlying(t);
            _tphConf.os_pres = m5::stl::to_underlying(p);
            _tphConf.os_hum  = m5::stl::to_underlying(h);
            return true;
        }
    }
    return false;
}

bool UnitBME688::setOversamplingTemperature(const bme688::Oversampling os) {
    uint8_t v{};
    if (readRegister8(CTRL_MEASUREMENT, v, 0)) {
        v = (v & ~((0x07 << 5) | 0x03)) | (m5::stl::to_underlying(os) << 5);
        if (writeRegister8(CTRL_MEASUREMENT, v)) {
            _tphConf.os_temp = m5::stl::to_underlying(os);
            return true;
        }
    }
    return false;
}

bool UnitBME688::setOversamplingPressure(const bme688::Oversampling os) {
    uint8_t v{};
    if (readRegister8(CTRL_MEASUREMENT, v, 0)) {
        v = (v & ~((0x07 << 2) | 0x03)) | (m5::stl::to_underlying(os) << 2);
        if (writeRegister8(CTRL_MEASUREMENT, v)) {
            _tphConf.os_pres = m5::stl::to_underlying(os);
            return true;
        }
    }
    return false;
}

bool UnitBME688::setOversamplingHumidity(const bme688::Oversampling os) {
    uint8_t v{};
    if (readRegister8(CTRL_HUMIDITY, v, 0)) {
        v = (v & ~0x07) | m5::stl::to_underlying(os);
        if (writeRegister8(CTRL_HUMIDITY, v)) {
            _tphConf.os_hum = m5::stl::to_underlying(os);
            return true;
        }
    }
    return false;
}

bool UnitBME688::setIIRFilter(const bme688::Filter f) {
    uint8_t v{};
    if (readRegister8(CONFIG, v, 0)) {
        v = (v & ~(0x07 << 2)) | (m5::stl::to_underlying(f) << 2);
        if (writeRegister8(CONFIG, v)) {
            _tphConf.filter = m5::stl::to_underlying(f);
            return true;
        }
    }
    return false;
}

bool UnitBME688::setHeaterSetting(const Mode mode,
                                  const bme688::bme68xHeatrConf& hs) {
    if (bme68x_set_heatr_conf(m5::stl::to_underlying(mode), &hs, &_dev) ==
        BME68X_OK) {
        _heaterConf = hs;
        return true;
    }
    return false;
}

bool UnitBME688::setMode(const Mode m) {
    if (bme68x_set_op_mode(m5::stl::to_underlying(m), &_dev) == BME68X_OK) {
        _mode = m;
        return true;
    }
    return false;
}

bool UnitBME688::readMode(Mode& m) {
    // int8_t bme68x_get_op_mode(uint8_t *op_mode, struct bme68x_dev *dev);
    return bme68x_get_op_mode((uint8_t*)&m, &_dev) == BME68X_OK;
}

uint32_t UnitBME688::calculateMeasurementInterval(const bme688::Mode mode,
                                                  const bme688::bme68xConf& s) {
    return bme68x_get_meas_dur(m5::stl::to_underlying(mode),
                               const_cast<struct bme68x_conf*>(&s), &_dev);
}

bool UnitBME688::measureSingleShot(bme68xData& data) {
    data = {};

    if (_bsec2_subscription) {
        M5_LIB_LOGW("During bsec2 operations");
        return false;
    }

    if (setMode(Mode::Forced)) {
        auto interval_us = calculateMeasurementInterval(_mode, _tphConf) +
                           (_heaterConf.heatr_dur * 1000);
        auto interval_ms = interval_us / 1000 + ((interval_us % 1000) != 0);
        m5::utility::delay(interval_ms);

        uint32_t retry{10};
        auto ret = read_measurement();
        while (!ret && retry--) {
            // M5_LIB_LOGE("ret:%u num:%u", ret, num);
            m5::utility::delay(1);
            ret = read_measurement();
        }
        if (ret) {
            data = _data[0];
            return readMode(_mode);
        }
    }
    return false;
}

bool UnitBME688::startPeriodicMeasurement(const Mode m) {
    _periodic = false;

    if (_bsec2_subscription) {
        M5_LIB_LOGW("During bsec2 operations");
        return false;
    }

    if (setMode(m)) {
        auto interval_us = calculateMeasurementInterval(_mode, _tphConf);
        switch (m) {
            case Mode::Forced:
                interval_us += _heaterConf.heatr_dur * 1000;
                break;
            case Mode::Parallel:
                interval_us += _heaterConf.shared_heatr_dur * 1000;
                break;
            case Mode::Sequential:
                interval_us += _heaterConf.heatr_dur_prof[0] * 1000;
                break;
            default:
                return false;
        }
        _interval = interval_us / 1000 + ((interval_us % 1000) != 0);
        // Always wait for an interval to obtain the correct value for the first
        // measurement
        _latest   = m5::utility::millis();
        _periodic = true;
    }
    return _periodic;
}

bool UnitBME688::stopPeriodicMeasurement() {
    if (_bsec2_subscription) {
        M5_LIB_LOGW("During bsec2 operations");
        return false;
    }

    if (setMode(Mode::Sleep)) {
        _periodic = false;
    }
    return !_periodic;
}

bool UnitBME688::read_measurement() {
    return bme68x_get_data(m5::stl::to_underlying(_mode), _data, &_num_of_data,
                           &_dev) == BME68X_OK;
}

#if defined(UNIT_BME688_USING_BSEC2)
float UnitBME688::latestData(const bsec_virtual_sensor_t vs) const {
    for (uint_fast8_t i = 0; i < _num_of_proccessed; ++i) {
        if (_processed[i].sensor_id == vs) {
            return _processed[i].signal;
        }
    }
    return std::numeric_limits<float>::quiet_NaN();
}

bool UnitBME688::bsec2GetConfig(uint8_t* cfg, uint32_t& actualSize) {
    return cfg && bsec_get_configuration(
                      0, cfg, BSEC_MAX_PROPERTY_BLOB_SIZE, _bsec2_work.get(),
                      BSEC_MAX_WORKBUFFER_SIZE, &actualSize) == BSEC_OK;
}

bool UnitBME688::bsec2SetConfig(const uint8_t* cfg) {
    if (cfg && bsec_set_configuration(cfg, BSEC_MAX_PROPERTY_BLOB_SIZE,
                                      _bsec2_work.get(),
                                      BSEC_MAX_WORKBUFFER_SIZE) == BSEC_OK) {
        return readCalibration(_dev.calib) && readTPHSetting(_tphConf);
    }
    return false;
}

bool UnitBME688::bsec2GetState(uint8_t* state, uint32_t& actualSize) {
    return state &&
           bsec_get_state(0, state, BSEC_MAX_STATE_BLOB_SIZE, _bsec2_work.get(),
                          BSEC_MAX_WORKBUFFER_SIZE, &actualSize) == BSEC_OK;
}

bool UnitBME688::bsec2SetState(const uint8_t* state) {
    return state &&
           bsec_set_state(state, BSEC_MAX_STATE_BLOB_SIZE, _bsec2_work.get(),
                          BSEC_MAX_WORKBUFFER_SIZE) == BSEC_OK;
}

bool UnitBME688::bsec2UpdateSubscription(const uint32_t sensorBits,
                                         const bme688::bsec2::SampleRate sr) {
    bsec_sensor_configuration_t vs[BSEC_NUMBER_OUTPUTS]{},
        ss[BSEC_MAX_PHYSICAL_SENSOR]{};
    uint8_t ssLen{BSEC_MAX_PHYSICAL_SENSOR};
    // idx 1:BSEC_OUTPUT_IAQ - 30:BSEC_OUTPUT_REGRESSION_ESTIMATE_4
    uint32_t idx{1}, num{0};
    const float rate{sample_rate_table[m5::stl::to_underlying(sr)]};
    while (idx < 31 && num < BSEC_NUMBER_OUTPUTS) {
        if (sensorBits & (1U << idx)) {
            vs[num].sensor_id   = idx;
            vs[num].sample_rate = rate;
            ++num;
        }
        ++idx;
    }
    // M5_DUMPI(vs, sizeof(bsec_sensor_configuration_t) * num);

    auto ret = bsec_update_subscription(vs, num, ss, &ssLen);
    if (ret == BSEC_OK) {
        // M5_DUMPI(ss, sizeof(bsec_sensor_configuration_t) * ssLen);
        _bsec2_subscription = sensorBits;
        _bsec2_sr           = sr;
        return true;
    }
    M5_LIB_LOGE("Failed to subscribe %d", ret);
    return false;
}

bool UnitBME688::bsec2Subscribe(const bsec_virtual_sensor_t id) {
    bsec_sensor_configuration_t vs[1]{}, ss[BSEC_MAX_PHYSICAL_SENSOR]{};
    uint8_t ssLen{BSEC_MAX_PHYSICAL_SENSOR};

    vs[0].sensor_id   = id;
    vs[0].sample_rate = sample_rate_table[m5::stl::to_underlying(_bsec2_sr)];
    if (bsec_update_subscription(vs, 1, ss, &ssLen) == BSEC_OK) {
        _bsec2_subscription |= 1U << id;
        return true;
    }
    return false;
}

bool UnitBME688::bsec2Unsubscribe(const bsec_virtual_sensor_t id) {
    bsec_sensor_configuration_t vs[1]{}, ss[BSEC_MAX_PHYSICAL_SENSOR]{};
    uint8_t ssLen{BSEC_MAX_PHYSICAL_SENSOR};

    vs[0].sensor_id   = id;
    vs[0].sample_rate = BSEC_SAMPLE_RATE_DISABLED;
    if (bsec_update_subscription(vs, 1, ss, &ssLen) == BSEC_OK) {
        _bsec2_subscription &= ~(1U << id);
        return true;
    }
    return false;
}

bool UnitBME688::bsec2UnsubscribeAll() {
    std::vector<bsec_sensor_configuration_t> v{};
    for (uint8_t i = 1; i < 32; ++i) {
        if (_bsec2_subscription & (1U << i)) {
            bsec_sensor_configuration_t bsc{BSEC_SAMPLE_RATE_DISABLED, i};
            v.push_back(bsc);
        }
    }
    bsec_sensor_configuration_t ss[BSEC_MAX_PHYSICAL_SENSOR]{};
    uint8_t ssLen{BSEC_MAX_PHYSICAL_SENSOR};
    if (bsec_update_subscription(v.data(), v.size(), ss, &ssLen) == BSEC_OK) {
        _bsec2_subscription = 0;
        return true;
    }
    return false;
}

//
bool UnitBME688::set_forced_mode() {
    bme68xHeatrConf hs{};
    hs.enable     = true;
    hs.heatr_temp = _bsec2_settings.heater_temperature;
    hs.heatr_dur  = _bsec2_settings.heater_duration;
    return setOversampling(
               (Oversampling)_bsec2_settings.temperature_oversampling,
               (Oversampling)_bsec2_settings.pressure_oversampling,
               (Oversampling)_bsec2_settings.humidity_oversampling) &&
           setHeaterSetting(Mode::Forced, hs) && setMode(Mode::Forced);
}

bool UnitBME688::set_parallel_mode() {
    uint16_t shared{};
    bme68xHeatrConf hs{};
    constexpr uint16_t BSEC_TOTAL_HEAT_DUR{140};

    shared = BSEC_TOTAL_HEAT_DUR -
             (calculateMeasurementInterval(Mode::Parallel, _tphConf) / 1000);

    hs.enable      = _bsec2_settings.heater_profile_len > 0;
    hs.profile_len = _bsec2_settings.heater_profile_len;
    memcpy(hs.heatr_temp_prof, _bsec2_settings.heater_temperature_profile,
           sizeof(uint16_t) * hs.profile_len);
    memcpy(hs.heatr_dur_prof, _bsec2_settings.heater_duration_profile,
           sizeof(uint16_t) * hs.profile_len);
    hs.shared_heatr_dur = shared;

    return setOversampling(
               (Oversampling)_bsec2_settings.temperature_oversampling,
               (Oversampling)_bsec2_settings.pressure_oversampling,
               (Oversampling)_bsec2_settings.humidity_oversampling) &&
           setHeaterSetting(Mode::Parallel, hs) && setMode(Mode::Parallel);
}

bool UnitBME688::fetch_data() {
    _num_of_data = 0;
    if (bme68x_get_data(m5::stl::to_underlying(_mode), _data, &_num_of_data,
                        &_dev) == BME68X_OK) {
        if (_mode == Mode::Forced) {
            _num_of_data = (_num_of_data >= 1) ? 1 : 0;  // 1 data if forced
        }
        return true;
    }
    return false;
}

// From bsec2.h
#define BSEC_CHECK_INPUT(x, shift) (x & (1 << (shift - 1)))

bool UnitBME688::process_data(const int64_t ns,
                              const bme688::bme68xData& data) {
    bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR]{}; /* Temp, Pres, Hum & Gas */
    uint8_t nInputs{0};
    /* Checks all the required sensor inputs, required for the BSEC library for
     * the requested outputs */
    if (BSEC_CHECK_INPUT(_bsec2_settings.process_data, BSEC_INPUT_HEATSOURCE)) {
        inputs[nInputs].sensor_id  = BSEC_INPUT_HEATSOURCE;
        inputs[nInputs].signal     = _temperatureOffset;
        inputs[nInputs].time_stamp = ns;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(_bsec2_settings.process_data,
                         BSEC_INPUT_TEMPERATURE)) {
#ifdef BME68X_USE_FPU
        inputs[nInputs].signal = data.temperature;
#else
        inputs[nInputs].signal = data.temperature / 100.0f;
#endif
        inputs[nInputs].sensor_id  = BSEC_INPUT_TEMPERATURE;
        inputs[nInputs].time_stamp = ns;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(_bsec2_settings.process_data, BSEC_INPUT_HUMIDITY)) {
#ifdef BME68X_USE_FPU
        inputs[nInputs].signal = data.humidity;
#else
        inputs[nInputs].signal = data.humidity / 1000.0f;
#endif
        inputs[nInputs].sensor_id  = BSEC_INPUT_HUMIDITY;
        inputs[nInputs].time_stamp = ns;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(_bsec2_settings.process_data, BSEC_INPUT_PRESSURE)) {
        inputs[nInputs].sensor_id  = BSEC_INPUT_PRESSURE;
        inputs[nInputs].signal     = data.pressure;
        inputs[nInputs].time_stamp = ns;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(_bsec2_settings.process_data,
                         BSEC_INPUT_GASRESISTOR) &&
        (data.status & BME68X_GASM_VALID_MSK)) {
        inputs[nInputs].sensor_id  = BSEC_INPUT_GASRESISTOR;
        inputs[nInputs].signal     = data.gas_resistance;
        inputs[nInputs].time_stamp = ns;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(_bsec2_settings.process_data,
                         BSEC_INPUT_PROFILE_PART) &&
        (data.status & BME68X_GASM_VALID_MSK)) {
        inputs[nInputs].sensor_id = BSEC_INPUT_PROFILE_PART;
        inputs[nInputs].signal =
            (_bsec2_mode == BME68X_FORCED_MODE) ? 0 : data.gas_index;
        inputs[nInputs].time_stamp = ns;
        nInputs++;
    }

    if (nInputs > 0) {
        _processed.fill({});
        _num_of_proccessed = _processed.size();
        auto ret           = bsec_do_steps(inputs, nInputs, _processed.data(),
                                           &_num_of_proccessed);
        if (ret == BSEC_OK) {
            return true;
        }
    }
    return false;
}
#endif

}  // namespace unit
}  // namespace m5
