/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_BME688.hpp
  @brief BME688 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ENV_UNIT_BME688_HPP
#define M5_UNIT_ENV_UNIT_BME688_HPP

#if (defined(ESP_PLATFORM) && (!defined(CONFIG_IDF_TARGET_ESP32C6) &&                                      \
                               (!defined(ARDUINO_M5Stack_NanoC6) && !defined(ARDUINO_M5STACK_NANOC6)))) || \
    defined(DOXYGEN_PROCESS)
#pragma message "Using BSEC2"
#define UNIT_BME688_USING_BSEC2
#endif

#include <M5UnitComponent.hpp>
#include <m5_utility/stl/extension.hpp>

#if defined(ARDUINO)
#include <bme68xLibrary.h>
#else
#include <bme68x/bme68x.h>
#endif

#if defined(UNIT_BME688_USING_BSEC2)
#if defined(ARDUINO)
#include <bsec2.h>
#else
#include <inc/bsec_datatypes.h>
#endif

#endif
#include <memory>
#include <limits>
#include <initializer_list>

namespace m5 {
namespace unit {

namespace bme688 {

/*!
  @enum Mode
  @brief Operation mode same as BME68X_xxx_MODE
 */
enum class Mode : uint8_t {
    Sleep,       //!<  No measurements are performed
    Forced,      //!< Single TPHG cycle is performed
    Parallel,    //!< Multiple TPHG cycles are performed
    Sequential,  //!< Sequential mode is similar as forced mode, but it provides
                 //!< temperature, pressure, humidity one by one
};

///@name Aliases for bme68x structures
///@{
/*!
  @typedef bme68xData
  @brief Raw data
 */
using bme68xData = struct bme68x_data;
/*!
  @typedef bme68xDev
  @brief bme68x device
 */
using bme68xDev = struct bme68x_dev;
/*!
  @typedef bme68xConf
  @brief Setting for temperature, pressure, humidity...
*/
using bme68xConf = struct bme68x_conf;
/*!
  @struct bme68xHeatrConf
  @brief Setting for gas heater
  @note Always valid pointers in derivation
*/
struct bme68xHeatrConf : bme68x_heatr_conf {
    uint16_t temp_prof[10]{};
    uint16_t dur_prof[10]{};
    bme68xHeatrConf() : bme68x_heatr_conf()
    {
        heatr_temp_prof = temp_prof;
        heatr_dur_prof  = dur_prof;
    }
};
/*!
  @typedef bme68xCalibration
  @brief Calibration parameters
 */
using bme68xCalibration = struct bme68x_calib_data;
///@}

/*!
  @enum Oversamping
  @brief Sampling setting
 */
enum class Oversampling : uint8_t {
    None,  //!< Skipped
    x1,    //!< x1
    x2,    //!< x2
    x4,    //!< x4
    x8,    //!< x8
    x16,   //!< x16
};

/*!
  @enum Filter
  @brief IIR Filter setting
 */
enum class Filter : uint8_t {
    None,
    Coeff_1,
    Coeff_3,
    Coeff_7,
    Coeff_15,
    Coeff_31,
    Coeff_63,
    Coeff_127,
};

/*!
  @enum ODR
  @brief bme68xConf::odr settings (standbytime Unit:ms)
 */
enum class ODR : uint8_t {
    MS_0_59,  //< 0.59 ms
    MS_62_5,  //< 62.5 ms
    MS_125,   //< 125 ms
    MS_250,   //!< 250 ms
    MS_500,   //!< 500 ms
    MS_1000,  //!< 1000 ms
    MS_10,    //!< 10 ms
    MS_20,    //!< 20 ms
    None,     //!< No standbytime
};

/*!
  @struct GasWait
  @brief GasSensor heater-on time
  @note Forced mode and Parallel mode are interpreted differently
 */
struct GasWait {
    GasWait() : value{0}
    {
    }
    /*!
      @enum Factor
      @brief Multiplier in Forced mode
     */
    enum class Factor { x1, x4, x16, x64 };
    ///@name Getter
    ///@{
    inline uint8_t step() const
    {
        return value & 0x3F;
    }
    inline Factor factor() const
    {
        return static_cast<Factor>((value >> 6) & 0x03);
    }
    ///@}

    ///@name Setter
    ///@{
    inline void step(const uint8_t s)
    {
        value = (value & ~0x3F) | (s & 0x3F);
    }
    inline void factor(const Factor f)
    {
        value = (value & ~(0x03 << 6)) | (m5::stl::to_underlying(f) << 6);
    }
    ///@}

    //! @brief Conversion from duration to register value for Force/Sequencial
    //! mode
    static uint8_t from(const uint16_t duration)
    {
        uint8_t f{};
        uint16_t d{duration};
        while (d > 0x3F) {
            d >>= 2;
            ++f;
        }
        return (f <= 0x03) ? ((uint8_t)d | (f << 6)) : 0xFF;
    }
    //! @brief Conversion from register value to duration for Force/Sequencial
    //! mode
    static uint16_t to(const uint8_t v)
    {
        constexpr uint16_t tbl[] = {1, 4, 16, 64};
        return (v & 0x3F) * tbl[(v >> 6) & 0x03];
    }

    uint8_t value{};  //!< Use the value as it is in parallel mode
};

#if defined(UNIT_BME688_USING_BSEC2)
namespace bsec2 {

/*!
  @enum SampleRate
  @brief Sample rate for BSEC2
 */
enum class SampleRate : uint8_t {
    Disabled,                          //!< Sample rate of a disabled sensor
    LowPower,                          //!< Sample rate in case of Low Power Mode (0.33Hz) interval 3 sec.
    UltraLowPower,                     //!< Sample rate in case of Ultra Low Power Mode (3.3 mHz) interval 300 sec.
    UltraLowPowerMeasurementOnDemand,  //!< Input value used to trigger an extra measurment (ULP plus) (0.33Hz
                                       //!< [T,p,h] 3.3mHz [IAQ])
    Scan,                              //!< Sample rate in case of scan mode (1/10.8 s)
    Continuous,                        //!< Sample rate in case of Continuous Mode (1Hz) interval 1 sec.
};

///@cond
template <typename T>
void is_bsec_virtual_sensor_t()
{
    static_assert(std::is_same<T, bsec_virtual_sensor_t>::value, "Argument must be of type bsec_virtual_sensor_t");
}
///@endcond

//! @brief  Conversion from BSEC2 subscription array to bits
inline uint32_t virtual_sensor_array_to_bits(const bsec_virtual_sensor_t* ss, const size_t len)
{
    uint32_t ret{};
    for (size_t i = 0; i < len; ++i) {
        ret |= ((uint32_t)1U) << ss[i];
    }
    return ret;
}

/*!
  @brief Make subscribe bits from bsec_virtual_sensor_t
 */
template <typename... Args>
uint32_t subscribe_to_bits(Args... args)
{
    // In a C++17 or later environment, it can be written like this...
    // static_assert(std::conjunction<std::is_same<Args, bsec_virtual_sensor_t>...>::value,
    //          "All arguments must be of type bsec_virtual_sensor_t");
    int discard[] = {(is_bsec_virtual_sensor_t<Args>(), 0)...};
    (void)discard;

    bsec_virtual_sensor_t tmp[] = {args...};
    constexpr size_t n          = sizeof...(args);
    return virtual_sensor_array_to_bits(tmp, n);
}

}  // namespace bsec2
#endif

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    bme688::bme68xData raw{};
#if defined(UNIT_BME688_USING_BSEC2)
    bsecOutputs raw_outputs{};

    float get(const bsec_virtual_sensor_t vs) const;
    inline float iaq() const
    {
        return get(BSEC_OUTPUT_IAQ);
    }
    inline float static_iaq() const
    {
        return get(BSEC_OUTPUT_STATIC_IAQ);
    }
    inline float co2() const
    {
        return get(BSEC_OUTPUT_CO2_EQUIVALENT);
    }
    inline float voc() const
    {
        return get(BSEC_OUTPUT_BREATH_VOC_EQUIVALENT);
    }
    inline float temperature() const
    {
        return get(BSEC_OUTPUT_RAW_TEMPERATURE);
    }
    inline float pressure() const
    {
        return get(BSEC_OUTPUT_RAW_PRESSURE);
    }
    inline float humidity() const
    {
        return get(BSEC_OUTPUT_RAW_HUMIDITY);
    }
    inline float gas() const
    {
        return get(BSEC_OUTPUT_RAW_GAS);
    }
    inline bool gas_stabilization() const
    {
        return get(BSEC_OUTPUT_STABILIZATION_STATUS) == 1.0f;
    }
    inline bool gas_run_in_status() const
    {
        return get(BSEC_OUTPUT_RUN_IN_STATUS) == 1.0f;
    }
    inline float heat_compensated_temperature() const
    {
        return get(BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE);
    }
    inline float heat_compensated_humidity() const
    {
        return get(BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY);
    }
    inline float gas_percentage() const
    {
        return get(BSEC_OUTPUT_GAS_PERCENTAGE);
    }
    inline float gas_estimate_1() const
    {
        return get(BSEC_OUTPUT_GAS_ESTIMATE_1);
    }
    inline float gas_estimate_2() const
    {
        return get(BSEC_OUTPUT_GAS_ESTIMATE_2);
    }
    inline float gas_estimate_3() const
    {
        return get(BSEC_OUTPUT_GAS_ESTIMATE_3);
    }
    inline float gas_estimate_4() const
    {
        return get(BSEC_OUTPUT_GAS_ESTIMATE_4);
    }
    inline uint32_t gas_index() const
    {
        return get(BSEC_OUTPUT_RAW_GAS_INDEX);
    }
    inline float regression_estimate_1() const
    {
        return get(BSEC_OUTPUT_REGRESSION_ESTIMATE_1);
    }
    inline float regression_estimate_2() const
    {
        return get(BSEC_OUTPUT_REGRESSION_ESTIMATE_2);
    }
    inline float regression_estimate_3() const
    {
        return get(BSEC_OUTPUT_REGRESSION_ESTIMATE_3);
    }
    inline float regression_estimate_4() const
    {
        return get(BSEC_OUTPUT_REGRESSION_ESTIMATE_4);
    }
#endif
    inline float raw_temperature() const
    {
        return raw.temperature;
    }
    inline float raw_pressure() const
    {
        return raw.pressure;
    }
    inline float raw_humidity() const
    {
        return raw.humidity;
    }
    inline float raw_gas() const
    {
        return raw.gas_resistance;
    }
};

}  // namespace bme688

/*!
  @class UnitBME688
  @brief BME688 unit
  @note Using config/bme688/bme688_sel_33v_3s_4d/bsec_selectivity.txt for default configuration
  @note If other settings are used, call bsec2SetConfig
 */
class UnitBME688 : public Component, public PeriodicMeasurementAdapter<UnitBME688, bme688::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitBME688, 0x77);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! ambient temperature
        int8_t ambient_temperature{25};
#if defined(UNIT_BME688_USING_BSEC2) || defined(DOXYGEN_PROCESS)
        ///@name  Exclude NanoC6
        ///@{
        /*! @brief Subscribe BSEC2 sensors bits if start on begin */
        uint32_t subscribe_bits{1U << BSEC_OUTPUT_IAQ | 1U << BSEC_OUTPUT_RAW_TEMPERATURE |
                                1U << BSEC_OUTPUT_RAW_PRESSURE | 1U << BSEC_OUTPUT_RAW_HUMIDITY |
                                1U << BSEC_OUTPUT_RAW_GAS | 1U << BSEC_OUTPUT_STABILIZATION_STATUS |
                                1U << BSEC_OUTPUT_RUN_IN_STATUS};
        /*!
          @brief Sampling rate for BSEC2 if start on begin
          @warning Depending on the value specified for the sample rate, appropriate configuration settings may be
          required in advance.
        */
        bme688::bsec2::SampleRate sample_rate{bme688::bsec2::SampleRate::LowPower};
#endif
#if !defined(UNIT_BME688_USING_BSEC2) || defined(DOXYGEN_PROCESS)
        ///@name Only Nano6
        ///@{
        /*! @brief Measurement mode if start on begin */
        bme688::Mode mode{bme688::Mode::Forced};
        //! Temperature oversampling if start on begin
        bme688::Oversampling oversampling_temperature{bme688::Oversampling::x2};
        //! Pressure oversampling if start on begin
        bme688::Oversampling oversampling_pressure{bme688::Oversampling::x1};
        //! Humidity oversampling if start on begin
        bme688::Oversampling oversampling_humidity{bme688::Oversampling::x16};
        //! Filter coefficient if start on begin
        bme688::Filter filter{bme688::Filter::None};
        //! Standby time between sequential mode measurement profiles if start on begin
        bme688::ODR odr{bme688::ODR::None};
        //! Enable gas measurement if start on begin
        bool heater_enable{true};
        //! The heater temperature for forced mode degree Celsius if start on begin
        uint16_t heater_temperature{300};
        //! The heating duration for forced mode in milliseconds if start on begin
        uint16_t heater_duration{100};
        ///@}
#endif
    };

    ///@name Configuration for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    explicit UnitBME688(const uint8_t addr = DEFAULT_ADDRESS);
    virtual ~UnitBME688()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Properties
    ///@{
    //! @brief Current mode
    inline bme688::Mode mode() const
    {
        return _mode;
    }
    //!@brief Gets the Calibration
    inline const bme688::bme68xCalibration& calibration() const
    {
        return _dev.calib;
    }
    //! @brief Gets the TPH setting
    inline const bme688::bme68xConf& tphSetting() const
    {
        return _tphConf;
    }
    //! @brief Gets the heater setiing
    inline const bme688::bme68xHeatrConf& heaterSetting() const
    {
        return _heaterConf;
    }
    //! @brief Gets the ambient temperature
    inline int8_t ambientTemperature() const
    {
        return _dev.amb_temp;
    }
    ///@}

    ///@name Measurement data by periodic
    ///@{
#if defined(UNIT_BME688_USING_BSEC2)
    /*!
      @brief Oldest measured IAQ
      @warning Exclude NanoC6
    */
    inline float iaq() const
    {
        return !empty() ? oldest().iaq() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured temperature (Celsius)
    inline float temperature() const
    {
        return !empty() ? oldest().temperature() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured pressure (Pa)
    inline float pressure() const
    {
        return !empty() ? oldest().pressure() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured humidity (%)
    inline float humidity() const
    {
        return !empty() ? oldest().humidity() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured gas (Ohm)
    inline float gas() const
    {
        return !empty() ? oldest().gas() : std::numeric_limits<float>::quiet_NaN();
    }
#else
    //! @brief Oldest measured temperature (Celsius)
    inline float temperature() const
    {
        return !empty() ? oldest().raw_temperature() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured pressure (Pa)
    inline float pressure() const
    {
        return !empty() ? oldest().raw_pressure() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured humidity (%)
    inline float humidity() const
    {
        return !empty() ? oldest().raw_humidity() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured gas (Ohm)
    inline float gas() const
    {
        return !empty() ? oldest().raw_gas() : std::numeric_limits<float>::quiet_NaN();
    }
#endif
    ///@}

#if 0
    /*!
      @brief Number of valid latest raw data
      @return between 0 and 3
      @note 0 or 1 in Forced mode
    */
    inline uint8_t numberOfRawData() const
    {
        return _num_of_data;
    }
    /*!
      @brief Obtaining the specified latest raw data pointer
      @param idx What number of the data
      @retval != nullptr Data pointer
      @retval == nullptr Failed
      @warning If it is not a valid range, nullptr is returned
     */
    inline const bme688::bme68xData* data(const uint8_t idx)
    {
        return (idx < _num_of_data) ? &_raw_data[idx] : nullptr;
    }
#endif

    //! @brief Sets the ambient temperature
    inline void setAambientTemperature(const int8_t temp)
    {
        _dev.amb_temp = temp;
    }
    /*!
      @brief Read calibration
      @param[out] c output value
      @return True if successful
     */
    bool readCalibration(bme688::bme68xCalibration& c);
    /*!
      @brief write calibration
      @param c Calibration parameter
      @return True if successful
     */
    bool writeCalibration(const bme688::bme68xCalibration& c);
    /*!
      @brief Calculation of measurement intervals without heater
      @return interval time (Unit: us)
    */
    uint32_t calculateMeasurementInterval(const bme688::Mode mode, const bme688::bme68xConf& s);
    /*!
      @brief Read unique ID
      @param[out] id output value
      @return True if successful
    */
    bool readUniqueID(uint32_t& id);
    /*!
      @brief Software reset
      @return True if successful
     */
    bool softReset();
    /*!
      @brief Self-test
      @return True if successful
    */
    bool selfTest();
    /*!
      @brief Write operation mode
      @param m Mode
      @return True if successful
     */
    bool writeMode(const bme688::Mode m);
    /*!
      @brief Read operation mode
      @param[out] m Mode
      @return True if successful
     */
    bool readMode(bme688::Mode& m);

    ///@name TPH(Temperature,Pressure,Humidity)
    ///@{
    /*!
      @brief Read TPH setting
      @param[out] s output value
      @return True if successful
    */
    bool readTPHSetting(bme688::bme68xConf& s);
    /*!
      @brief Read temperature oversampling
      @param[out] os output value
      @return True if successful
     */
    bool readOversamplingTemperature(bme688::Oversampling& os);
    /*!
      @brief Read pressure oversampling
      @param[out] os output value
      @return True if successful
     */
    bool readOversamplingPressure(bme688::Oversampling& os);
    /*!
      @brief Read IIRFilter
      @param[out] f output value
      @return True if successful
     */
    bool readIIRFilter(bme688::Filter& f);
    /*!
      @brief Read humidity oversampling
      @param[out] os output value
      @return True if successful
     */
    bool readOversamplingHumidity(bme688::Oversampling& os);
    /*!
      @brief Write TPH setting
      @param s Setting
      @return True if successful
    */
    bool writeTPHSetting(const bme688::bme68xConf& s);
    /*!
      @brief Wite oversamplings
      @param t oversampling for temperature
      @param p oversampling for pressure
      @param h oversampling for humidity
    */
    bool writeOversampling(const bme688::Oversampling t, const bme688::Oversampling p, const bme688::Oversampling h);
    /*!
      @brief Write temperature oversampling
      @param os enum value
      @return True if successful
     */
    bool writeOversamplingTemperature(const bme688::Oversampling os);
    /*!
      @brief Write pressure oversampling
      @param os enum value
      @return True if successful
     */
    bool writeOversamplingPressure(const bme688::Oversampling os);
    /*!
      @brief Write humidity oversampling
      @param os enum value
      @return True if successful
     */
    bool writeOversamplingHumidity(const bme688::Oversampling os);
    /*!
      @brief Write IIRFilter
      @param[out] f enum value
      @return True if successful
    */
    bool writeIIRFilter(const bme688::Filter f);
    ///@}

    ///@name Heater
    ///@{
    /*!
      @brief Read heater setting
      @param hs Setting
      @return True if successful
      @warning Only heatr_dur_prof and heatr_temp_prof can be obtained
     */
    bool readHeaterSetting(bme688::bme68xHeatrConf& hs);
    /*!
      @brief Write heater setting
      @param mode Expected operation mode of the sensor
      @param hs Setting
      @return True if successful
     */
    bool writeHeaterSetting(const bme688::Mode mode, const bme688::bme68xHeatrConf& hs);
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement without BSEC2
      @param m Mode for measurement
      @return True if successful
      @pre Calibration,TPH and heater must already be set up
      @warning Measurement intervals are not constant in Parallel mode
    */
    inline bool startPeriodicMeasurement(const bme688::Mode m)
    {
        return PeriodicMeasurementAdapter<UnitBME688, bme688::Data>::startPeriodicMeasurement(m);
    }
#if defined(UNIT_BME688_USING_BSEC2) || defined(DOXYGEN_PROCESS)
    /*!
      @brief Start periodic measurement using BSEC2
      @param subscribe_bits Measurement type bits
      @return True if successful
      @warning Not available for NanoC6
    */
    inline bool startPeriodicMeasurement(const uint32_t subscribe_bits,
                                         const bme688::bsec2::SampleRate sr = bme688::bsec2::SampleRate::LowPower)
    {
        return PeriodicMeasurementAdapter<UnitBME688, bme688::Data>::startPeriodicMeasurement(subscribe_bits, sr);
    }
    /*!
      @brief Start periodic measurement using BSEC2
      @param ss Array of requested virtual sensor (output) configurations for the library
      @param len Number of array elements
      @return True if successful
      @warning Not available for NanoC6
    */
    inline bool startPeriodicMeasurement(const bsec_virtual_sensor_t* ss, const size_t len,
                                         const bme688::bsec2::SampleRate sr = bme688::bsec2::SampleRate::LowPower)
    {
        return ss ? startPeriodicMeasurement(bme688::bsec2::virtual_sensor_array_to_bits(ss, len), sr) : false;
    }

#endif
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitBME688, bme688::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Take a single measurement
      @param[out] data output value
      @return True if successful
      @pre Calibration,TPH and heater must already be set up
      @note Measure once by Force mode
      @note Blocked until it can be measured.
    */
    bool measureSingleShot(bme688::bme68xData& data);
    ///@}

#if defined(UNIT_BME688_USING_BSEC2) || defined(DOXYGEN_PROCESS)
    ///@warning Not available for NanoC6
    ///@name Bosch BSEC2 wrapper
    ///@{
    /*!
      @brief Gets the temperature offset(Celsius)
     */
    float bsec2GetTemperatureOffset() const
    {
        return _temperatureOffset;
    }
    /*!
      @brief Set the temperature offset(Celsius)
      @param offset offset value
     */
    void bsec2SetTemperatureOffset(const float offset)
    {
        _temperatureOffset = offset;
    }
    /*!
      @brief Gets the BSEC2 library version
      @return reference of the version structure
      @warning Call after begin
    */
    const bsec_version_t& bsec2Version() const
    {
        return _bsec2_version;
    }
    /*!
      @brief Update algorithm configuration parameters
      Update bsec2 configuration settings
      @param cfg Settings serialized to a binary blob
      @param sz size of cfg
      @return True if successful
      @note See also BSEC2 src/config/bme688 for configuration data
    */
    bool bsec2SetConfig(const uint8_t* cfg, const size_t sz = BSEC_MAX_PROPERTY_BLOB_SIZE);
    /*!
      @brief Retrieve the current library configuration
      @param[out] cfg Buffer to hold the serialized config blob
      @param[out] actualSize Actual size of the returned serialized configuration blob
      @return True if successful
      @warning cfg size must be greater than or equal to BSEC_MAX_PROPERTY_BLOB_SIZE
    */
    bool bsec2GetConfig(uint8_t* cfg, uint32_t& actualSize);
    /*!
      @brief Restore the internal state
      @param state pointer of the state array
      @return True if successful
    */
    bool bsec2SetState(const uint8_t* state);
    /*!
      @brief Retrieve the current internal library state
      @param[out] state Buffer to hold the serialized state blob
      @param[out] actualSize Actual size of the returned serialized blob
      @return True if successful
      @warning cfg size must be greater than or equal to BSEC_MAX_STATE_BLOB_SIZE
    */
    bool bsec2GetState(uint8_t* state, uint32_t& actualSize);
    /*!
      @brief Subscribe to library virtual sensors outputs
      @param sensorBits Requested virtual sensor (output) configurations for the library
      @param sr Sample rate
      @return True if successful
      @warning Depending on the value specified for the sample rate, appropriate configuration settings may be
      required in advance.
    */
    bool bsec2UpdateSubscription(const uint32_t sensorBits, const bme688::bsec2::SampleRate sr);
    /*!
      @brief Subscribe to library virtual sensors outputs
      @param ss Array of requested virtual sensor (output) configurations for the library
      @param len Number of array elements
      @param sr Sample rate
      @return True if successful
      @warning Depending on the value specified for the sample rate, appropriate configuration settings may be
      required in advance.
    */
    inline bool bsec2UpdateSubscription(const bsec_virtual_sensor_t* ss, const size_t len,
                                        const bme688::bsec2::SampleRate sr)
    {
        return bsec2UpdateSubscription(bme688::bsec2::virtual_sensor_array_to_bits(ss, len), sr);
    }
    /*!
      @brief is virtual sensor Subscribed?
      @param id virtual sensor (output)
      @return True if subscribed
    */
    inline bool bsec2IsSubscribed(const bsec_virtual_sensor_t id)
    {
        return _bsec2_subscription & (1U << id);
    }
    /*!
      @brief Gets the subscription bits
      @return Subscribed sensor id bits
      @note If BSEC_OUTPUT_IAQ is subscribed, then bit 2 (means 1U << BSEC_OUTPUT_IAQ) is 1
     */
    uint32_t bsec2Subscription() const
    {
        return _bsec2_subscription;
    }
    /*!
      @brief Subscribe virtual sensor
      @param id virtual sensor (output)
      @return True if successful
      @note SampleRate uses the value that is currently set or has been set in the past
    */
    bool bsec2Subscribe(const bsec_virtual_sensor_t id);
    /*!
      @brief Unsubscribe virtual sensor
      @param id virtual sensor (output)
      @return True if successful
    */
    bool bsec2Unsubscribe(const bsec_virtual_sensor_t id);
    /*!
      @brief Unsubacribe currentt all sensors
      @return True if successful
     */
    bool bsec2UnsubscribeAll();
///@}
#endif

protected:
    static int8_t read_function(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr);
    static int8_t write_function(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr);

    bool start_periodic_measurement(const bme688::Mode m);
    bool stop_periodic_measurement();
#if defined(UNIT_BME688_USING_BSEC2)
    bool start_periodic_measurement(const uint32_t subscribe_bits, const bme688::bsec2::SampleRate sr);
#endif

    bool write_mode_forced();
    bool write_mode_parallel();
    bool fetch_data();

    void update_bme688(const bool force);
    bool read_measurement();
#if defined(UNIT_BME688_USING_BSEC2)
    bool process_data(bsecOutputs& outouts, const int64_t ns, const bme688::bme68xData& data);
    void update_bsec2(const bool force);
#endif

    inline virtual bool in_periodic() const override
    {
        return _periodic || (_bsec2_subscription != 0);
    }

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitBME688, bme688::Data);

protected:
    bme688::Mode _mode{bme688::Mode::Sleep};

    // bme68x
    bme688::bme68xData _raw_data[3]{};  // latest data
    uint8_t _num_of_data{};
    bme688::bme68xDev _dev{};
    bme688::bme68xConf _tphConf{};
    bme688::bme68xHeatrConf _heaterConf{};

    // BSEC2
    uint32_t _bsec2_subscription{};  // Enabled virtual sensor bit

#if defined(UNIT_BME688_USING_BSEC2)
    bsec_version_t _bsec2_version{};
    std::unique_ptr<uint8_t> _bsec2_work{};
    bsec_bme_settings_t _bsec2_settings{};

    bme688::Mode _bsec2_mode{};
    bme688::bsec2::SampleRate _bsec2_sr{};

    bsecOutputs _outputs{};
    float _temperatureOffset{};
#endif

    std::unique_ptr<m5::container::CircularBuffer<bme688::Data>> _data{};

    bool _waiting{};
    types::elapsed_time_t _can_measure_time{};

    config_t _cfg{};
};

///@cond 0
namespace bme688 {
namespace command {
constexpr uint8_t CHIP_ID{0xD0};
constexpr uint8_t RESET{0xE0};
constexpr uint8_t VARIANT_ID{0xF0};

constexpr uint8_t IDAC_HEATER_0{0x50};  // ...9
constexpr uint8_t RES_HEAT_0{0x5A};     // ...9
constexpr uint8_t GAS_WAIT_0{0x64};     // ...9
constexpr uint8_t GAS_WAIT_SHARED{0x6E};

constexpr uint8_t CTRL_GAS_0{0x70};
constexpr uint8_t CTRL_GAS_1{0x71};
constexpr uint8_t CTRL_HUMIDITY{0x72};
constexpr uint8_t CTRL_MEASUREMENT{0x74};
constexpr uint8_t CONFIG{0x75};

constexpr uint8_t MEASUREMENT_STATUS_0{0x1D};
constexpr uint8_t MEASUREMENT_STATUS_1{0x2E};
constexpr uint8_t MEASUREMENT_STATUS_2{0x3F};

constexpr uint8_t MEASUREMENT_GROUP_INDEX_0{0x1F};
constexpr uint8_t MEASUREMENT_GROUP_INDEX_1{0x30};
constexpr uint8_t MEASUREMENT_GROUP_INDEX_2{0x41};

constexpr uint8_t UNIQUE_ID{0x83};

// calibration
constexpr uint8_t CALIBRATION_GROUP_0{0x8A};
constexpr uint8_t CALIBRATION_GROUP_1{0xE1};
constexpr uint8_t CALIBRATION_GROUP_2{0x00};
constexpr uint8_t CALIBRATION_TEMPERATURE_1_LOW{0xE9};
constexpr uint8_t CALIBRATION_TEMPERATURE_2_LOW{0x8A};
constexpr uint8_t CALIBRATION_TEMPERATURE_3{0x8C};
constexpr uint8_t CALIBRATION_PRESSURE_1_LOW{0x8E};
constexpr uint8_t CALIBRATION_PRESSURE_2_LOW{0x90};
constexpr uint8_t CALIBRATION_PRESSURE_3{0x92};
constexpr uint8_t CALIBRATION_PRESSURE_4_LOW{0x94};
constexpr uint8_t CALIBRATION_PRESSURE_5_LOW{0x96};
constexpr uint8_t CALIBRATION_PRESSURE_6{0x99};
constexpr uint8_t CALIBRATION_PRESSURE_7{0x98};
constexpr uint8_t CALIBRATION_PRESSURE_8_LOW{0x9C};
constexpr uint8_t CALIBRATION_PRESSURE_9_LOW{0x9E};
constexpr uint8_t CALIBRATION_PRESSURE_10{0xA0};
constexpr uint8_t CALIBRATION_HUMIDITY_12{0xE2};
constexpr uint8_t CALIBRATION_HUMIDITY_1_HIGH{0xE3};
constexpr uint8_t CALIBRATION_HUMIDITY_2_HIGH{0xE1};
constexpr uint8_t CALIBRATION_HUMIDITY_3{0xE4};
constexpr uint8_t CALIBRATION_HUMIDITY_4{0xE5};
constexpr uint8_t CALIBRATION_HUMIDITY_5{0xE6};
constexpr uint8_t CALIBRATION_HUMIDITY_6{0xE7};
constexpr uint8_t CALIBRATION_HUMIDITY_7{0xE8};
constexpr uint8_t CALIBRATION_GAS_1{0xED};
constexpr uint8_t CALIBRATION_GAS_2_LOW{0xEB};
constexpr uint8_t CALIBRATION_GAS_3{0xEE};
constexpr uint8_t CALIBRATION_RES_HEAT_RANGE{0x02};  // [5:4]
constexpr uint8_t CALIBRATION_RES_HEAT_VAL{0x00};

}  // namespace command
}  // namespace bme688
///@endcond

}  // namespace unit
}  // namespace m5
#endif
