/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SCD40.hpp
  @brief SCD40 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ENV_UNIT_SCD40_HPP
#define M5_UNIT_ENV_UNIT_SCD40_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

/*!
  @namespace scd4x
  @brief For SCD40/41
 */
namespace scd4x {
/*!
  @enum Mode
  @brief Mode of periodic measurement
 */
enum class Mode : uint8_t {
    Normal,    //!< Normal (Receive data every 5 seconds)
    LowPower,  //!< Low power (Receive data every 30 seconds)
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 9> raw{};  //!< @brief RAW data
    uint16_t co2() const;          //!< @brief CO2 concentration (ppm)
    //! @brief temperature (Celsius)
    inline float temperature() const
    {
        return celsius();
    }
    float celsius() const;     //!< @brief temperature (Celsius)
    float fahrenheit() const;  //!< @brief temperature (Fahrenheit)
    float humidity() const;    //!< @brief humidity (RH)
};

///@cond
// Max command duration(ms)
// For SCD40/41
constexpr uint16_t READ_MEASUREMENT_DURATION{1};
constexpr uint16_t STOP_PERIODIC_MEASUREMENT_DURATION{500};
constexpr uint16_t SET_TEMPERATURE_OFFSET_DURATION{1};
constexpr uint16_t GET_TEMPERATURE_OFFSET_DURATION{1};
constexpr uint16_t SET_SENSOR_ALTITUDE_DURATION{1};
constexpr uint16_t GET_SENSOR_ALTITUDE_DURATION{1};
constexpr uint16_t SET_AMBIENT_PRESSURE_DURATION{1};
constexpr uint16_t PERFORM_FORCED_CALIBRATION_DURATION{400};
constexpr uint16_t SET_AUTOMATIC_SELF_CALIBRATION_ENABLED_DURATION{1};
constexpr uint16_t GET_AUTOMATIC_SELF_CALIBRATION_ENABLED_DURATION{1};
constexpr uint16_t GET_DATA_READY_STATUS_DURATION{1};
constexpr uint16_t PERSIST_SETTINGS_DURATION{800};
constexpr uint16_t GET_SERIAL_NUMBER_DURATION{1};
constexpr uint16_t PERFORM_SELF_TEST_DURATION{10000};
constexpr uint16_t PERFORM_FACTORY_RESET_DURATION{1200};
constexpr uint16_t REINIT_DURATION{20};
/// @endcond

}  // namespace scd4x

/*!
  @class UnitSCD40
  @brief SCD40 unit component
*/
class UnitSCD40 : public Component, public PeriodicMeasurementAdapter<UnitSCD40, scd4x::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSCD40, 0x62);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Mode of periodic measurement if start on begin?
        scd4x::Mode mode{scd4x::Mode::Normal};
        //! Enable calibration on begin?
        bool calibration{true};
    };

    explicit UnitSCD40(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<scd4x::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitSCD40()
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
    inline void config(const config_t &cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest measured CO2 concentration (ppm)
    inline uint16_t co2() const
    {
        return !empty() ? oldest().co2() : 0;
    }
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
      @param mode Measurement mode
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const scd4x::Mode mode = scd4x::Mode::Normal)
    {
        return PeriodicMeasurementAdapter<UnitSCD40, scd4x::Data>::startPeriodicMeasurement(mode);
    }
    /*!
      @brief Start low power periodic measurement
      @return True if successful
    */
    inline bool startLowPowerPeriodicMeasurement()
    {
        return startPeriodicMeasurement(scd4x::Mode::LowPower);
    }
    /*!
      @brief Stop periodic measurement
      @param duration Max command duration(ms)
      @return True if successful
    */
    inline bool stopPeriodicMeasurement(const uint32_t duration = scd4x::STOP_PERIODIC_MEASUREMENT_DURATION)
    {
        return PeriodicMeasurementAdapter<UnitSCD40, scd4x::Data>::stopPeriodicMeasurement(duration);
    }
    ///@}

    ///@name On-Chip Output Signal Compensation
    ///@{
    /*!
      @brief Write the temperature offset
      @details Define how warm the sensor is compared to ambient, so RH and T
      are temperature compensated. Has no effect on the CO2 reading Default offsetis 4C
      @param offset (0 <= offset < 175)
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeTemperatureOffset(const float offset, const uint32_t duration = scd4x::SET_TEMPERATURE_OFFSET_DURATION);
    /*!
      @brief Read the temperature offset
      @param[out] offset Offset value
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool readTemperatureOffset(float &offset);
    /*!
      @brief Write the sensor altitude
      @details Define the sensor altitude in metres above sea level, so RH and CO2 arecompensated for atmospheric
      pressure Default altitude is 0m
      @param altitude Unit:metres
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeSensorAltitude(const uint16_t altitude, const uint32_t duration = scd4x::SET_SENSOR_ALTITUDE_DURATION);
    /*!
      @brief Read the sensor altitude
      @param[out] altitude Altitude value
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool readSensorAltitude(uint16_t &altitude);
    /*!
      @brief Write the ambient pressure
      @details Define the ambient pressure in Pascals, so RH and CO2 are compensated for atmospheric pressure
      setAmbientPressure overrides setSensorAltitude
      @param presure Unit: pascals (>= 0.0f)
      @param duration Max command duration(ms)
      @return True if successful
    */
    bool writeAmbientPressure(const float pressure, const uint32_t duration = scd4x::SET_AMBIENT_PRESSURE_DURATION);
    ///@}

    ///@name Field Calibration
    ///@{
    /*!
      @brief Perform forced recalibration
      @param concentration Unit:ppm
      @param[out] correction The FRC correction value
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning  Blocked until the process is completed (about 400ms)
    */
    bool performForcedRecalibration(const uint16_t concentration, int16_t &correction);
    /*!
      @brief Enable/disable automatic self calibration
      @param enabled Enable automatic self calibration if true
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeAutomaticSelfCalibrationEnabled(
        const bool enabled = true, const uint32_t duration = scd4x::SET_AUTOMATIC_SELF_CALIBRATION_ENABLED_DURATION);
    /*!
      @brief Check if automatic self calibration is enabled
      @param[out] enabled  True if automatic self calibration is enabled
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Measurement duration max 1 ms
    */
    bool readAutomaticSelfCalibrationEnabled(bool &enabled);
    ///@}

    ///@name Advanced Features
    ///@{
    /*!
      @brief Write sensor settings from RAM to EEPROM
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writePersistSettings(const uint32_t duration = scd4x::PERSIST_SETTINGS_DURATION);
    /*!
      @brief Read the serial number string
      @param[out] serialNumber Output buffer
      @return True if successful
      @warning Size must be at least 13 bytes
      @warning During periodic detection runs, an error is returned
    */
    bool readSerialNumber(char *serialNumber);
    /*!
      @brief Read the serial number value
      @param[out] serialNumber serial number value
      @return True if successful
      @note The serial number is 48 bit
      @warning During periodic detection runs, an error is returned
    */
    bool readSerialNumber(uint64_t &serialNumber);
    /*!
      @brief Perform self test
      @param[out] True if malfunction detected
      @return True if successful
      @note Takes 10 seconds to complete
      @warning During periodic detection runs, an error is returned
    */
    bool performSelfTest(bool &malfunction);
    /*!
      @brief Peform factory reset
      @details Reset all settings to the factory values
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Measurement duration max 1200 ms
    */
    bool performFactoryReset(const uint32_t duration = scd4x::PERFORM_FACTORY_RESET_DURATION);
    /*!
      @brief Re-initialize the sensor, load settings from EEPROM
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Measurement duration max 20 ms
    */
    bool reInit(const uint32_t duration = scd4x::REINIT_DURATION);
    ///@}

protected:
    bool start_periodic_measurement(const scd4x::Mode mode = scd4x::Mode::Normal);
    bool stop_periodic_measurement(const uint32_t duration = scd4x::STOP_PERIODIC_MEASUREMENT_DURATION);
    bool read_data_ready_status();
    bool read_measurement(scd4x::Data &d, const bool all = true);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitSCD40, scd4x::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<scd4x::Data>> _data{};
    config_t _cfg{};
};

///@cond
namespace scd4x {
namespace command {
// Basic Commands
constexpr uint16_t START_PERIODIC_MEASUREMENT{0x21b1};
constexpr uint16_t READ_MEASUREMENT{0xec05};
constexpr uint16_t STOP_PERIODIC_MEASUREMENT{0x3f86};
// On-chip output signal compensation
constexpr uint16_t SET_TEMPERATURE_OFFSET{0x241d};
constexpr uint16_t GET_TEMPERATURE_OFFSET{0x2318};
constexpr uint16_t SET_SENSOR_ALTITUDE{0x2427};
constexpr uint16_t GET_SENSOR_ALTITUDE{0x2322};
constexpr uint16_t SET_AMBIENT_PRESSURE{0xe000};
// Field calibration
constexpr uint16_t PERFORM_FORCED_CALIBRATION{0x362f};
constexpr uint16_t SET_AUTOMATIC_SELF_CALIBRATION_ENABLED{0x2416};
constexpr uint16_t GET_AUTOMATIC_SELF_CALIBRATION_ENABLED{0x2313};
// Low power
constexpr uint16_t START_LOW_POWER_PERIODIC_MEASUREMENT{0x21ac};
constexpr uint16_t GET_DATA_READY_STATUS{0xe4b8};
// Advanced features
constexpr uint16_t PERSIST_SETTINGS{0x3615};
constexpr uint16_t GET_SERIAL_NUMBER{0x3682};
constexpr uint16_t PERFORM_SELF_TEST{0x3639};
constexpr uint16_t PERFORM_FACTORY_RESET{0x3632};
constexpr uint16_t REINIT{0x3646};
}  // namespace command
}  // namespace scd4x
///@endcond

}  // namespace unit
}  // namespace m5
#endif
