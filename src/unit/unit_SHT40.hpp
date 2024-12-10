/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SHT40.hpp
  @brief SHT40 Unit for M5UnitUnified
 */
#ifndef M5_UNIT_ENV_UNIT_SHT40_HPP
#define M5_UNIT_ENV_UNIT_SHT40_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

/*!
  @namespace sht40
  @brief For SHT40
 */
namespace sht40 {

/*!
  @enum Precision
  @brief precision level
 */
enum class Precision : uint8_t {
    High,    //!< High precision (high repeatability)
    Medium,  //!< Medium precision (medium repeatability)
    Low      //!< Lowest precision (low repeatability)
};

/*!
  @enum Heater
  @brief Heater behavior
 */
enum class Heater : uint8_t {
    Long,   //!< Activate heater for 1s
    Short,  //!< Activate heater for 0.1s
    None    //!< Not activate heater
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 6> raw{};  //!< RAW data
    bool heater{};                 //!< Measured data after heater is activated if true

    //! temperature (Celsius)
    inline float temperature() const
    {
        return celsius();
    }
    float celsius() const;     //!< temperature (Celsius)
    float fahrenheit() const;  //!< temperature (Fahrenheit)
    float humidity() const;    //!< humidity (RH)
};

}  // namespace sht40

/*!
  @class UnitSHT40
  @brief Temperature and humidity, sensor unit
*/
class UnitSHT40 : public Component, public PeriodicMeasurementAdapter<UnitSHT40, sht40::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSHT40, 0x44);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Precision level if start on begin
        sht40::Precision precision{sht40::Precision::High};
        //! Heater behavior if start on begin
        sht40::Heater heater{sht40::Heater::None};
        //! Heater duty cycle if start on begin [~ 0.05f]
        float heater_duty{0.05f};
    };

    explicit UnitSHT40(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<sht40::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitSHT40()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
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

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest measured temperature (Celsius)
    inline float temperature() const
    {
        return !empty() ? oldest().temperature() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured temperature (Celsius)
    inline float celsius() const
    {
        return !empty() ? oldest().celsius() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured temperature (Fahrenheit)
    inline float fahrenheit() const
    {
        return !empty() ? oldest().fahrenheit() : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured humidity (RH)
    inline float humidity() const
    {
        return !empty() ? oldest().humidity() : std::numeric_limits<float>::quiet_NaN();
    }
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @param precision Sensor precision
      @param heater Heater behavior
      @param duty Duty for activate heater
      @return True if successful
      @note If the heater is Long or SHort, the heater will be active periodically within the specified duty
      @warning Datasheet says "keepingin mind that the heater is designed for a maximal duty cycle of less than 5%"
    */
    inline bool startPeriodicMeasurement(const sht40::Precision precision, const sht40::Heater heater,
                                         const float duty = 0.05f)
    {
        return PeriodicMeasurementAdapter<UnitSHT40, sht40::Data>::startPeriodicMeasurement(precision, heater, duty);
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
     */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitSHT40, sht40::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @param[out] data Measuerd data
      @param precision Sensor precision
      @param heater Heater behavior
      @return True if successful
      @note Blocking until the process is complete
      @warning During periodic detection runs, an error is returned
      @warning If heater is activated, the accuracy of the returned value is not guaranteed
      @sa UnitSHT40::startPeriodicMeasurement
    */
    bool measureSingleshot(sht40::Data& d, const sht40::Precision precision = sht40::Precision::High,
                           const sht40::Heater heater = sht40::Heater::None);

    ///@}

    ///@name Reset
    ///@{
    /*!
      @brief Soft reset
      @return True if successful
    */
    bool softReset();
    /*!
      @brief General reset
      @details Reset using I2C general call
      @return True if successful
      @warning This is a reset by General command, the command is also sent to all devices with I2C connections
    */
    bool generalReset();
    ///@}

    ///@name Serial number
    ///@{
    /*!
      @brief Read the serial number value
      @param[out] serialNumber serial number value
      @return True if successful
      @note The serial number is 32 bit
      @warning During periodic detection runs, an error is returned
    */
    bool readSerialNumber(uint32_t& serialNumber);
    /*!
      @brief Read the serial number string
      @param[out] serialNumber Output buffer
      @return True if successful
      @warning serialNumber must be at least 9 bytes
      @warning During periodic detection runs, an error is returned
    */
    bool readSerialNumber(char* serialNumber);
    ///@}

protected:
    bool start_periodic_measurement(const sht40::Precision precision, const sht40::Heater heater, const float duty);
    bool stop_periodic_measurement();
    bool read_measurement(sht40::Data& d);
    void reset_status();
    bool soft_reset();

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitSHT40, sht40::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<sht40::Data>> _data{};
    config_t _cfg{};
    uint8_t _cmd{}, _measureCmd{};
    types::elapsed_time_t _latest_heater{}, _interval_heater{};
    uint32_t _duration_measure{}, _duration_heater{};
};

///@cond
namespace sht40 {
namespace command {
constexpr uint8_t MEASURE_HIGH_HEATER_1S{0x39};
constexpr uint8_t MEASURE_HIGH_HEATER_100MS{0x32};
constexpr uint8_t MEASURE_HIGH{0xFD};

constexpr uint8_t MEASURE_MEDIUM_HEATER_1S{0x2F};
constexpr uint8_t MEASURE_MEDIUM_HEATER_100MS{0x24};
constexpr uint8_t MEASURE_MEDIUM{0xF6};

constexpr uint8_t MEASURE_LOW_HEATER_1S{0x1E};
constexpr uint8_t MEASURE_LOW_HEATER_100MS{0x15};
constexpr uint8_t MEASURE_LOW{0xE0};

constexpr uint8_t GET_SERIAL_NUMBER{0x89};
constexpr uint8_t SOFT_RESET{0x94};
}  // namespace command
}  // namespace sht40
///@endcond
}  // namespace unit
}  // namespace m5
#endif
