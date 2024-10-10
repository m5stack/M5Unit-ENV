/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SGP30.hpp
  @brief SGP30 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_TVOC_UNIT_SGP30_HPP
#define M5_UNIT_TVOC_UNIT_SGP30_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <array>

namespace m5 {
namespace unit {

/*!
  @namespace sgp30
  @brief For SGP30
 */
namespace sgp30 {

///@cond
// Max command duration(ms)
// Max measurement duration (ms)
constexpr uint16_t IAQ_INIT_DURATION{10};
constexpr uint16_t MEASURE_IAQ_DURATION{12};
constexpr uint16_t GET_IAQ_BASELINE_DURATION{10};
constexpr uint16_t SET_IAQ_BASELINE_DURATION{10};
constexpr uint16_t SET_ABSOLUTE_HUMIDITY_DURATION{10};
constexpr uint16_t MEASURE_TEST_DURATION{220};
constexpr uint16_t GET_FEATURE_SET_DURATION{10};
constexpr uint16_t MEASURE_RAW_DURATION{25};
constexpr uint16_t GET_TVOC_INCEPTIVE_BASELINE_DURATION{10};
constexpr uint16_t SET_TVOC_INCEPTIVE_BASELINE_DURATION{10};
constexpr uint16_t GET_SERIAL_ID_DURATION{10};
///@endcond

/*!
  @struct Feature
  @brief Structure of the SGP feature set number
 */
struct Feature {
    //! @brief product type (SGP30: 0)
    uint8_t productType() const
    {
        return (value >> 12) & 0x0F;
    }
    /*!
      @brief product version
      @note Please note that the last 5 bits of the productversion (bits 12-16
      of the LSB) are subject to change
      @note This is used to track new features added tothe SGP multi-pixel
      platform
     */
    uint8_t productVersion() const
    {
        return value & 0xFF;
    }
    uint16_t value{};
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 6> raw{};  //!< RAW data
    uint16_t co2eq() const;        //!< Co2Eq (ppm)
    uint16_t tvoc() const;         //!< TVOC (pbb)
};

}  // namespace sgp30

/*!
  @class UnitSGP30
  @brief SGP30 unit
 */
class UnitSGP30 : public Component, public PeriodicMeasurementAdapter<UnitSGP30, sgp30::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSGP30, 0x58);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin()?
        bool start_periodic{true};
        //!  Baseline CO2eq initial value if start on begin
        uint16_t baseline_co2eq{};
        //! Baseline TVOC initial value if start on begin
        uint16_t baseline_tvoc{};
        //! Absolute humidity initiali value if start on begin
        uint16_t humidity{};
        /*!
          Inceptive Baseline for TVOC measurements initial value if start on begin
          @warning The application of this feature is solely limited to the very
          first start-up period of an SGP sensor
         */
        uint16_t inceptive_tvoc{};
        //! Periodic measurement interval if start on begin
        int32_t interval{1000};
    };

    explicit UnitSGP30(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<sgp30::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitSGP30()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config() const
    {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@name Properties
    ///@{
    /*!
      @brief Gets the product version
      @warning Calling after the call of begin()
     */
    inline uint8_t productVersion() const
    {
        return _version;
    }
    /*!
      @brief Can it be measured?
      @return True if it can
      @warning After the start of a periodic measurement, it is necessary to wait 15 seconds to obtain a valid
      measurement value
     */
    inline bool canMeasurePeriodic() const
    {
        return inPeriodic() && !_waiting;
    }
    ///@}

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest CO2eq (ppm)
    inline uint16_t co2eq() const
    {
        return !empty() ? oldest().co2eq() : 0xFFFF;
    }
    //! @brief Oldest TVOC (ppb)
    inline uint16_t tvoc() const
    {
        return !empty() ? oldest().tvoc() : 0xFFFF;
    }
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @details Specify settings and measure
      @param co2eq iaq baseline for CO2eq
      @param tvoc iaq baseline for TVOC
      @param humidity absolute humidity (disable if zero)
      @param interval Measurement Interval(ms)
      @param duration Max command duration(ms)
      @return True if successful
      @note 15 seconds wait is required before a valid measurement can be initiated (warmup)
      @note In update(), waiting are taken into account
    */
    inline bool startPeriodicMeasurement(const uint16_t co2eq, const uint16_t tvoc, const uint16_t humidity,
                                         const uint32_t interval = 1000U,
                                         const uint32_t duration = sgp30::IAQ_INIT_DURATION)
    {
        return PeriodicMeasurementAdapter<UnitSGP30, sgp30::Data>::startPeriodicMeasurement(co2eq, tvoc, humidity,
                                                                                            interval, duration);
    }
    /*!
      @brief Start periodic measurement
      @details Measuring in the current settings
      @param interval Measurement Interval(ms)
      @param duration Max command duration(ms)
      @return True if successful
      @note 15 seconds wait is required before a valid measurement can be
      initiated.
      @note In update(), waiting are taken into account
    */
    inline bool startPeriodicMeasurement(const uint32_t interval = 1000U,
                                         const uint32_t duration = sgp30::IAQ_INIT_DURATION)
    {
        return PeriodicMeasurementAdapter<UnitSGP30, sgp30::Data>::startPeriodicMeasurement(interval, duration);
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitSGP30, sgp30::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Correction
    ///@{
    /*!
      @brief Read the IAQ baseline
      @param[out] co2eq iaq baseline for CO2
      @param[out] tvoc iaq baseline for TVOC
      @return True if successful
    */
    bool readIaqBaseline(uint16_t& co2eq, uint16_t& tvoc);
    /*!
      @brief Write the absolute humidity
      @param raw absolute humidity (disable if zero)
      @param duration Max command duration(ms)
      @return True if successful
      @note Value is a fixed-point 8.8bit number
    */
    bool writeAbsoluteHumidity(const uint16_t raw, const uint32_t duration = sgp30::SET_ABSOLUTE_HUMIDITY_DURATION);
    /*!
      @brief Write the absolute humidity
      @param gm3 absolute humidity (g/m^3)
      @param duration Max command duration(ms)
      @return True if successful
    */
    bool writeAbsoluteHumidity(const float gm3, const uint32_t duration = sgp30::SET_ABSOLUTE_HUMIDITY_DURATION);

#if 0
    /*!
      @brief Read the inceptive Basebine for TVOC
      @param[out] inceptive_tvoc Inceptive baseline
      @return True if successful
      @warning Only available if product version is 0x21 or higher
    */
    bool readTvocInceptiveBaseline(uint16_t& inceptive_tvoc);
    /*!
      @brief Write the inceptive Basebine for TVOC
      @param inceptive_tvoc Inceptive baseline
      @param duration Max command duration(ms)
      @return True if successful
      @warning The application of this feature is solely limited to the very
      first start-up period of an SGP sensor
      @warning Only available if product version is 0x21 or higher
    */
    bool writeTvocInceptiveBaseline(const uint16_t inceptive_tvoc,
                                    const uint32_t duration = sgp30::SET_TVOC_INCEPTIVE_BASELINE_DURATION);
#endif
    ///@}

    /*!
      @brief Read sensor raw signals
      @param[out] h2 H2 concentration(raw)
      @param[out] ethanol Ethanol concentration(raw)
      @return True if successful
    */
    bool readRaw(uint16_t& h2, uint16_t& ethanol);

    /*!
      @brief Read H2/Ethanol concentration
      @param[out] h2 H2 concentration(ppm)
      @param[out] ethanol Ethanol concentration(rppm
      @return True if successful
      @note Outputs the value calculated from the output of readRaw
    */
    bool readRaw(float& h2, float& ethanol);

    /*!
      @brief Run the on-chip self-test
      @param[out] result self-test return code
      @return True if successful
      @note If the test is OK, the result will be 0xd400
      @warning Must not be executed after startPeriodicMeasurement
    */
    bool measureTest(uint16_t& result);
    /*!
       @brief General reset
       @details Reset using I2C general call
       @warning This is a reset by General command, the command is also
       sent to all devices with I2C connections
       @return True if successful
     */
    bool generalReset();
    /*!
      @brief Read the feature set
      @param feature
      @return True if successful
     */
    bool readFeatureSet(sgp30::Feature& feature);
    /*!
      @brief Read the serial number
      @return True if successful
      @note Serial number is 48bits
    */
    bool readSerialNumber(uint64_t& number);
    /*!
      @brief Read the serial number string
      @param[out] number Output buffer
      @return True if successful
      @warning number must be at least 13 bytes
    */
    bool readSerialNumber(char* number);

protected:
    bool start_periodic_measurement(const uint16_t co2eq, const uint16_t tvoc, const uint16_t humidity,
                                    const uint32_t interval, const uint32_t duration = sgp30::IAQ_INIT_DURATION);
    bool start_periodic_measurement(const uint32_t interval, const uint32_t duration = sgp30::IAQ_INIT_DURATION);
    bool stop_periodic_measurement();

    bool write_iaq_baseline(const uint16_t co2eq, const uint16_t tvoc);
    bool read_measurement(sgp30::Data& d);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitSGP30, sgp30::Data);

protected:
    uint8_t _version{};  // Chip version

    bool _waiting{};
    types::elapsed_time_t _can_measure_time{};
    std::unique_ptr<m5::container::CircularBuffer<sgp30::Data>> _data{};

    config_t _cfg{};
};

namespace sgp30 {
namespace command {
///@cond
constexpr uint16_t IAQ_INIT{0x2003};
constexpr uint16_t MEASURE_IAQ{0x2008};
constexpr uint16_t GET_IAQ_BASELINE{0x2015};
constexpr uint16_t SET_IAQ_BASELINE{0x201E};
constexpr uint16_t SET_ABSOLUTE_HUMIDITY{0x2061};
constexpr uint16_t MEASURE_TEST{0x2032};
constexpr uint16_t GET_FEATURE_SET{0x202F};
constexpr uint16_t MEASURE_RAW{0x2050};
constexpr uint16_t GET_TVOC_INCEPTIVE_BASELINE{0x20B3};
constexpr uint16_t SET_TVOC_INCEPTIVE_BASELINE{0x2077};
constexpr uint16_t GET_SERIAL_ID{0x3682};
///@endcond
}  // namespace command
}  // namespace sgp30

}  // namespace unit
}  // namespace m5
#endif
