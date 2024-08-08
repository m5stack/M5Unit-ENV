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
#include "internal/scd4x.hpp"
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

/*!
  @class UnitSCD40
  @brief SCD40 unit component
*/
class UnitSCD40 : public Component,
                  public PeriodicMeasurementAdapter<UnitSCD40, scd4x::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSCD40, 0x62);

   public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t : Component::config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Mode of periodic measurement if start on begin?
        scd4x::Mode mode{scd4x::Mode::Normal};
        //! Enable calibration on begin?
        bool calibration{true};
    };

    explicit UnitSCD40(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr),
          _data{new m5::container::CircularBuffer<scd4x::Data>(1)} {
        auto ccfg  = component_config();
        ccfg.clock = 400000U;
        component_config(ccfg);
    }
    virtual ~UnitSCD40() {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config() const {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t &cfg) {
        _cfg = cfg;
    }
    ///@}

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest measured CO2 concentration (ppm)
    inline uint16_t co2() const {
        return !empty() ? oldest().co2() : 0;
    }
    //! @brief Oldest measured temperature (Celsius)
    inline float temperature() const {
        return !empty() ? oldest().temperature()
                        : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured temperature (Celsius)
    inline float celsius() const {
        return !empty() ? oldest().celsius()
                        : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured temperature (Fahrenheit)
    inline float fahrenheit() const {
        return !empty() ? oldest().fahrenheit()
                        : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured humidity (RH)
    inline float humidity() const {
        return !empty() ? oldest().humidity()
                        : std::numeric_limits<float>::quiet_NaN();
    }
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start low power periodic measurements
      @return True if successful
    */
    inline bool startLowPowerPeriodicMeasurement() {
        return startPeriodicMeasurement(scd4x::Mode::LowPower);
    }
    ///@}
    
    ///@name On-Chip Output Signal Compensation
    ///@{
    /*!
      @brief Set the temperature offset
      @details Define how warm the sensor is compared to ambient, so RH and T
      are temperature compensated. Has no effect on the CO2 reading Default
      offsetis 4C
      @param offset (0 <= offset < 175)
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool setTemperatureOffset(
        const float offset,
        const uint32_t duration = scd4x::SET_TEMPERATURE_OFFSET_DURATION);
    /*!
      @brief Read the temperature offset
      @param[out] offset Offset value
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool readTemperatureOffset(float &offset);
    /*!
      @brief Set the sensor altitude
      @details Define the sensor altitude in metres above sea level, so RH and
      CO2 arecompensated for atmospheric pressure Default altitude is 0m
      @param altitude Unit:metres
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool setSensorAltitude(
        const uint16_t altitude,
        const uint32_t duration = scd4x::SET_SENSOR_ALTITUDE_DURATION);
    /*!
      @brief Read the sensor altitude
      @param[out] altitude Altitude value
      @param duration Max command duration(ms)
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool readSensorAltitude(uint16_t &altitude);
    /*!
      @brief Set the ambient pressure
      @details Define the ambient pressure in Pascals, so RH and CO2 are
      compensated for atmospheric pressure setAmbientPressure overrides
      setSensorAltitude
      @param presure Unit: pascals (>= 0.0f)
      @param duration Max command duration(ms)
      @return True if successful
    */
    bool setAmbientPressure(
        const float pressure,
        const uint32_t duration = scd4x::SET_AMBIENT_PRESSURE_DURATION);
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
    bool performForcedRecalibration(const uint16_t concentration,
                                    int16_t &correction);
    /*!
      @brief Enable/disable automatic self calibration
      @param enabled Enable automatic self calibration if true
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool setAutomaticSelfCalibrationEnabled(
        const bool enabled = true,
        const uint32_t duration =
            scd4x::SET_AUTOMATIC_SELF_CALIBRATION_ENABLED_DURATION);
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
      @brief Copy sensor settings from RAM to EEPROM
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool persistSettings(
        const uint32_t duration = scd4x::PERSIST_SETTINGS_DURATION);
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
    bool performFactoryReset(
        const uint32_t duration = scd4x::PERFORM_FACTORY_RESET_DURATION);
    /*!
      @brief Re-initialize the sensor, load settings from EEPROM
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Measurement duration max 20 ms
    */
    bool reInit(const uint32_t duration = scd4x::REINIT_DURATION);
    ///@}

   protected:
    ///@note Call via startPeriodicMeasurement/stopPeriodicMeasurement
    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @param mode Mode of periodic measurement
      @return True if successful
    */
    bool start_periodic_measurement(
        const scd4x::Mode mode = scd4x::Mode::Normal);
    /*!
      @brief Stop measurement
      @param duration Max command duration(ms)
      @warning The sensor will only respond to other commands after waiting 500
      ms after issuing the stop_periodic_measurement command
      @return True if successful
    */
    bool stop_periodic_measurement(
        const uint32_t duration = scd4x::STOP_PERIODIC_MEASUREMENT_DURATION);
    ///@}

    bool read_data_ready_status();
    bool read_measurement(scd4x::Data &d, const bool all = true);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitSCD40,
                                                               scd4x::Data);

   protected:
    std::unique_ptr<m5::container::CircularBuffer<scd4x::Data>> _data{};
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5
#endif
