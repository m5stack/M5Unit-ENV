/*!
  @file unit_BME688.hpp
  @brief BME688 Unit for M5UnitUnified

  SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD

  SPDX-License-Identifier: MIT

*/
#ifndef M5_UNIT_ENV_UNIT_BME688_HPP
#define M5_UNIT_ENV_UNIT_BME688_HPP

#if (defined(ESP_PLATFORM) && (!defined(CONFIG_IDF_TARGET_ESP32C6) && \
                               !defined(ARDUINO_M5Stack_NanoC6))) ||  \
    defined(DOXYGEN_PROCESS)
#pragma message "Using BSEC2"
#define UNIT_BME688_USING_BSEC2
#endif

#include <M5UnitComponent.hpp>
#include <m5_utility/stl/extension.hpp>
#include <bme68x/bme68x.h>
#if defined(UNIT_BME688_USING_BSEC2)
#include <inc/bsec_datatypes.h>
#endif
#include <array>
#include <memory>
#include <limits>

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
  @note  Deriverd from bme68x_heatr_conf, so that the pointer is always valid
*/
struct bme68xHeatrConf : bme68x_heatr_conf {
    uint16_t temp_prof[10]{};
    uint16_t dur_prof[10]{};
    bme68xHeatrConf() : bme68x_heatr_conf() {
        heatr_temp_prof = temp_prof;
        heatr_dur_prof  = dur_prof;
    }
};
/*!
  @typedef Calibration
  @brief Calibration parameters
 */
using Calibration = struct bme68x_calib_data;
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
    GasWait() : value{0} {
    }
    /*!
      @enum Factor
      @brief Multiplier in Forced mode
     */
    enum class Factor { x1, x4, x16, x64 };
    ///@name Getter
    ///@{
    inline uint8_t step() const {
        return value & 0x3F;
    }
    inline Factor factor() const {
        return static_cast<Factor>((value >> 6) & 0x03);
    }
    ///@}

    ///@name Setter
    ///@{
    inline void step(const uint8_t s) {
        value = (value & ~0x3F) | (s & 0x3F);
    }
    inline void factor(const Factor f) {
        value = (value & ~(0x03 << 6)) | (m5::stl::to_underlying(f) << 6);
    }
    ///@}

    //! @brief Conversion from duration to register value for Force/Sequencial
    //! mode
    static uint8_t from(const uint16_t duration) {
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
    static uint16_t to(const uint8_t v) {
        constexpr uint16_t tbl[] = {1, 4, 16, 64};
        return (v & 0x3F) * tbl[(v >> 6) & 0x03];
    }

    uint8_t value{};  //!< Use the value as it is in parallel mode
};

#if defined(UNIT_BME688_USING_BSEC2)
namespace bsec2 {

//! @brief  Conversion from BSEC2 subscription to bits
inline uint32_t virtual_sensor_array_to_bits(const bsec_virtual_sensor_t* ss,
                                             const size_t len) {
    uint32_t ret{};
    for (size_t i = 0; i < len; ++i) {
        ret |= ((uint32_t)1U) << ss[i];
    }
    return ret;
}

/*!
  @enum SampleRate
  @brief Sample rate for BSEC2
 */
enum class SampleRate : uint8_t {
    Disabled,
    LowPower,
    UltraLowPower,
    UltraLowPowerMeasurementOnDemand,
    Scan,
    Continuous,
};

}  // namespace bsec2
#endif

}  // namespace bme688

/*!
  @class UnitBME688
  @brief BME688 unit
 */
class UnitBME688 : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitBME688, 0x77);

   public:
    /*!
      @struct config_t
      @brief Settings
     */
    struct config_t {
        //        //! @brief Start periodic measurement on begin?
        //        bool start_periodic{true};
        //! @brief Settings for measuerment
        //        Setting setting{};
        //! @brief initial operation mode
        // bme688::Mode mode{bme688::Mode::Forced};
        // float temperature_offset{};
    };

    ///@name Configuration for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config() {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t& cfg) {
        _cfg = cfg;
    }
    ///@}

    explicit UnitBME688(const uint8_t addr = DEFAULT_ADDRESS);
    virtual ~UnitBME688() {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Properties
    ///@{
    //! @brief Current mode
    inline bme688::Mode mode() const {
        return _mode;
    }
    //!@brief Gets the Calibration
    inline const bme688::Calibration& calibration() const {
        return _dev.calib;
    }
    //! @brief Gets the TPH setting
    inline const bme688::bme68xConf& tphSetting() const {
        return _tphConf;
    }
    //! @brief Gets the heater setiing
    inline const bme688::bme68xHeatrConf& heaterSetting() const {
        return _heaterConf;
    }
    //! @brief Gets the ambient temperature
    inline int8_t ambientTemperature() const {
        return _dev.amb_temp;
    }
#if defined(UNIT_BME688_USING_BSEC2)
    /*!
      @brief Get the latest value of the specified output
      @param vs bsec_virtual_sensor_t
      @retval != nan Latest data
      @retval nan Not subscribed
    */
    float latestData(const bsec_virtual_sensor_t vs) const;
    //! @brief latest temperature (If there is more than one data set, the first
    //! one)
    inline float temperature() const {
        return _bsec2_subscription ? latestData(BSEC_OUTPUT_RAW_TEMPERATURE)
                                   : _data[0].temperature;
    }
    //! @brief latest pressure (If there is more than one data set, the first
    //! one)
    inline float pressure() const {
        return _bsec2_subscription ? latestData(BSEC_OUTPUT_RAW_PRESSURE)
                                   : _data[0].pressure;
    }
    //! @brief latest humidity (If there is more than one data set, the first
    //! one)
    inline float humidity() const {
        return _bsec2_subscription ? latestData(BSEC_OUTPUT_RAW_HUMIDITY)
                                   : _data[0].humidity;
    }
    //! @brief latest gas resistance (If there is more than one data set, the
    //! first one)
    inline float resistance() const {
        return _bsec2_subscription ? latestData(BSEC_OUTPUT_RAW_GAS)
                                   : _data[0].gas_resistance;
    }
    //! @brief latest IAQ if subscribed
    inline float IAQ() const {
        return latestData(BSEC_OUTPUT_IAQ);
    }
#else
    //! @brief latest temperature (If there is more than one data set, the first
    //! one)
    inline float temperature() const {
        return _data[0].temperature;
    }
    //! @brief latest pressure (If there is more than one data set, the first
    //! one)
    inline float pressure() const {
        return _data[0].pressure;
    }
    //! @brief latest humidity (If there is more than one data set, the first
    //! one)
    inline float humidity() const {
        return _data[0].humidity;
    }
    //! @brief latest gas resistance (If there is more than one data set, the
    //! first one)
    inline float resistance() const {
        return _data[0].gas_resistance;
    }
#endif
    /*!
      @brief Number of valid latest raw data
      @return between 0 and 3
      @note 0 or 1 in Forced mode
    */
    inline uint8_t numberOfRawData() const {
        return _num_of_data;
    }
    /*!
      @brief Obtaining the specified latest raw data pointer
      @param idx What number of the data
      @retval != nullptr Data pointer
      @retval == nullptr Failed
      @warning If it is not a valid range, nullptr is returned
     */
    inline const bme688::bme68xData* data(const uint8_t idx) {
        return (idx < _num_of_data) ? &_data[idx] : nullptr;
    }
    ///@}

    //! @brief Sets the ambient temperature
    inline void setAambientTemperature(const int8_t temp) {
        _dev.amb_temp = temp;
    }
    /*!
      @brief Read calibration
      @param[out] c output value
      @return True if successful
     */
    bool readCalibration(bme688::Calibration& c);
    /*!
      @brief set calibration
      @param c Calibration parameter
      @return True if successful
     */
    bool setCalibration(const bme688::Calibration& c);
    /*!
      @brief Calculation of measurement intervals without heater
      @return interval time (Unit: us)
    */
    uint32_t calculateMeasurementInterval(const bme688::Mode mode,
                                          const bme688::bme68xConf& s);
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
      @brief Set operation mode
      @param m Mode
      @return True if successful
     */
    bool setMode(const bme688::Mode m);
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
      @brief Set TPH setting
      @param s Setting
      @return True if successful
    */
    bool setTPHSetting(const bme688::bme68xConf& s);
    /*!
      @brief Set oversamplings
      @param t oversampling for temperature
      @param p oversampling for pressure
      @param h oversampling for humidity
    */
    bool setOversampling(const bme688::Oversampling t,
                         const bme688::Oversampling p,
                         const bme688::Oversampling h);
    /*!
      @brief Set temperature oversampling
      @param os enum value
      @return True if successful
     */
    bool setOversamplingTemperature(const bme688::Oversampling os);
    /*!
      @brief Set pressure oversampling
      @param os enum value
      @return True if successful
     */
    bool setOversamplingPressure(const bme688::Oversampling os);
    /*!
      @brief Set humidity oversampling
      @param os enum value
      @return True if successful
     */
    bool setOversamplingHumidity(const bme688::Oversampling os);
    /*!
      @brief Set IIRFilter
      @param[out] f enum value
      @return True if successful
    */
    bool setIIRFilter(const bme688::Filter f);
    ///@}

    /*!
      @brief Set heater setting
      @param mode Expected operation mode of the sensor
      @param hs Setting
      @return True if successful
     */
    bool setHeaterSetting(const bme688::Mode mode,
                          const bme688::bme68xHeatrConf& hs);

    ///@name Using bme688 directly
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
    /*!
      @brief Start periodic measurement
      @param m Mode for measurement
      @return True if successful
      @pre Calibration,TPH and heater must already be set up
      @warning Measurement intervals are not constant in Parallel mode
    */
    bool startPeriodicMeasurement(const bme688::Mode m);
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    bool stopPeriodicMeasurement();
    ///@}

#if defined(UNIT_BME688_USING_BSEC2)
    ///@name BSEC2 (Not support ESP32C6)
    ///@brief for bsec2 library
    ///@{
    /*!
      @brief Gets the temperature offset(Celsius)
     */
    float bsec2GetTemperatureOffset() const {
        return _temperatureOffset;
    }
    /*!
      @brief Set the temperature offset(Celsius)
      @param offset offset value
     */
    void bsec2SetTemperatureOffset(const float offset) {
        _temperatureOffset = offset;
    }
    /*!
      @brief Gets the BSEC2 library version
      @return reference of the version structure
      @warning Call after begin
    */
    const bsec_version_t& bsec2Version() const {
        return _bsec2_version;
    }
    /*!
      @brief Update algorithm configuration parameters
      Update bsec2 configuration settings
      @param cfg Settings serialized to a binary blob
      @return True if successful
      @note See src/config/bme688/bme688_sel_33v_....txt in the BSEC2 library
      for configuration data
    */
    bool bsec2SetConfig(const uint8_t* cfg);
    /*!
      @brief Retrieve the current library configuration
      @param[out] cfg Buffer to hold the serialized config blob
      @param[out] actualSize Actual size of the returned serialized
      configuration blob
      @return True if successful
      @warning cfg size must be greater than or equal to
      BSEC_MAX_PROPERTY_BLOB_SIZE
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
      @warning cfg size must be greater than or equal to
      BSEC_MAX_STATE_BLOB_SIZE
    */
    bool bsec2GetState(uint8_t* state, uint32_t& actualSize);
    /*!
      @brief Subscribe to library virtual sensors outputs
      @param sensorBits Requested virtual sensor (output) configurations for the
      library
      @param sr Sample rate
      @return True if successful
    */
    bool bsec2UpdateSubscription(const uint32_t sensorBits,
                                 const bme688::bsec2::SampleRate sr);
    /*!
      @brief Subscribe to library virtual sensors outputs
      @param ss Array of requested virtual sensor (output) configurations for
      the library
      @param len Length of ss
      @param sr Sample rate
      @return True if successful
    */
    inline bool bsec2UpdateSubscription(const bsec_virtual_sensor_t* ss,
                                        const size_t len,
                                        const bme688::bsec2::SampleRate sr) {
        return bsec2UpdateSubscription(
            bme688::bsec2::virtual_sensor_array_to_bits(ss, len), sr);
    }
    /*!
      @brief is virtual sensor Subscribed?
      @param vs virtual sensor
      @return True if subscribed
     */
    inline bool bsec2IsSubscribed(const bsec_virtual_sensor_t id) {
        return _bsec2_subscription & (1U << id);
    }
    /*!
      @brief Gets the subscription bits
      @return Subscribed sensor id bits
      @note If BSEC_OUTPUT_IAQ is subscribed, then bit 2 (means 1U <<
      BSEC_OUTPUT_IAQ) is 1
     */
    bool bsec2Subscription() const {
        return _bsec2_subscription;
    }
    /*!
      @brief Subscribe virtual sensor
      @param vs virtual sensor
      @return True if successful
      @note SampleRate uses the value that is currently set or has been set in
      the past
    */
    bool bsec2Subscribe(const bsec_virtual_sensor_t id);
    /*!
      @brief Unsubscribe virtual sensor
      @param vs virtual sensor
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
    static int8_t read_function(uint8_t reg_addr, uint8_t* reg_data,
                                uint32_t length, void* intf_ptr);
    static int8_t write_function(uint8_t reg_addr, const uint8_t* reg_data,
                                 uint32_t length, void* intf_ptr);

    bool set_forced_mode();
    bool set_parallel_mode();
    bool fetch_data();
    bool process_data(const int64_t ns, const bme688::bme68xData& data);

    void update_bsec2(const bool force);
    void update_bme688(const bool force);
    bool read_measurement();

   protected:
    bme688::Mode _mode{};

    // bme68x
    bme688::bme68xData _data[3]{};  // latest data
    uint8_t _num_of_data{};
    bme688::bme68xDev _dev{};
    bme688::bme68xConf _tphConf{};
    bme688::bme68xHeatrConf _heaterConf{};

    // BSEC2
    uint32_t _bsec2_subscription{};  // Enabled virtual sensor bit (0-30)
#if defined(UNIT_BME688_USING_BSEC2)
    bsec_version_t _bsec2_version{};
    std::unique_ptr<uint8_t> _bsec2_work{};
    bme688::bsec2::SampleRate _bsec2_sr{};
    bsec_bme_settings_t _bsec2_settings{};
    bme688::Mode _bsec2_mode{};
    std::array<bsec_output_t, 32> _processed{};
    uint8_t _num_of_proccessed{};
    float _temperatureOffset{};
#endif

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
