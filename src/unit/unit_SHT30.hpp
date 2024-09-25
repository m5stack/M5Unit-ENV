/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SHT30.hpp
  @brief SHT30 Unit for M5UnitUnified
 */
#ifndef M5_UNIT_ENV_UNIT_SHT30_HPP
#define M5_UNIT_ENV_UNIT_SHT30_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

namespace sht30 {
/*!
  @enum Repeatability
  @brief Repeatability accuracy level
 */
enum class Repeatability : uint8_t {
    High,    //!< @brief High repeatability
    Medium,  //!< @brief Medium repeatability
    Low      //!< @brief Low repeatability
};

/*!
  @enum MPS
  @brief Measuring frequency
 */
enum class MPS : uint8_t {
    Half,  //!< @brief 0.5 measurement per second
    One,   //!< @brief 1 measurement per second
    Two,   //!< @brief 2 measurement per second
    Four,  //!< @brief 4 measurement per second
    Ten,   //!< @brief 10 measurement per second
};

/*!
  @struct Status
  @brief Accessor for Status
  @note The order of the bit fields cannot be controlled, so bitwise
  operations are used to obtain each value.
  @note Items marked with (*) are subjects to clear status
 */
struct Status {
    //! @brief Alert pending status (*)
    inline bool alertPending() const
    {
        return value & (1U << 15);
    }
    //! @brief Heater status
    inline bool heater() const
    {
        return value & (1U << 13);
    }
    //! @brief RH tracking alert (*)
    inline bool trackingAlertRH() const
    {
        return value & (1U << 11);
    }
    //! @brief Tracking alert (*)
    inline bool trackingAlert() const
    {
        return value & (1U << 10);
    }
    //! @brief System reset detected (*)
    inline bool reset() const
    {
        return value & (1U << 4);
    }
    //! @brief Command staus
    inline bool command() const
    {
        return value & (1U << 1);
    }
    //! @brief Write data checksum status
    inline bool checksum() const
    {
        return value & (1U << 0);
    }
    uint16_t value{};
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 6> raw{};  //!< RAW data
    //! temperature (Celsius)
    inline float temperature() const
    {
        return celsius();
    }
    float celsius() const;     //!< temperature (Celsius)
    float fahrenheit() const;  //!< temperature (Fahrenheit)
    float humidity() const;    //!< humidity (RH)
};

}  // namespace sht30

/*!
  @class UnitSHT30
  @brief Temperature and humidity, sensor unit
*/
class UnitSHT30 : public Component, public PeriodicMeasurementAdapter<UnitSHT30, sht30::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSHT30, 0x44);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Measuring frequency if start on begin
        sht30::MPS mps{sht30::MPS::One};
        //! Repeatability accuracy level if start on begin
        sht30::Repeatability repeatability{sht30::Repeatability::High};
        //! start heater on begin?
        bool start_heater{false};
    };

    explicit UnitSHT30(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<sht30::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitSHT30()
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
      @param mps Measuring frequency
      @param rep Repeatability accuracy level
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const sht30::MPS mps, const sht30::Repeatability rep)
    {
        return PeriodicMeasurementAdapter<UnitSHT30, sht30::Data>::startPeriodicMeasurement(mps, rep);
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
     */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitSHT30, sht30::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @param[out] data Measuerd data
      @param rep Repeatability accuracy level
      @param stretch Enable clock stretching if true
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning  After sending a command to the sensor a minimal waiting time of 1ms is needed before another command can
      be received by the sensor
    */
    bool measureSingleshot(sht30::Data& d, const sht30::Repeatability rep = sht30::Repeatability::High,
                           const bool stretch = true);
    ///@}

    /*!
      @brief Write the mode to ART
      @details After issuing the ART command the sensor will start acquiring data with a frequency of 4Hz
      @return True if successful
      @warning Only available during periodic measurements
    */
    bool writeModeAccelerateResponseTime();

    ///@name Reset
    ///@{
    /*!
      @brief Soft reset
      @details The sensor to reset its system controller and reloads calibration
      data from the memory.
      @return True if successful
      @warning During periodic detection runs, an error is returned
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

    ///@name Heater
    ///@{
    /*!
      @brief Start heater
      @return True if successful
     */
    bool startHeater();
    /*!
      @brief Stop heater
      @return True if successful
     */
    bool stopHeater();
    ///@}

    ///@name Status
    ///@{
    /*!
      @brief Read status
      @param[out] s Status
      @return True if successful
    */
    bool readStatus(sht30::Status& s);
    /*!
      @brief Clear status
      @sa sht30::Status
      @return True if successful
    */
    bool clearStatus();
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
    bool start_periodic_measurement(const sht30::MPS mps, const sht30::Repeatability rep);
    bool stop_periodic_measurement();
    bool read_measurement(sht30::Data& d);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitSHT30, sht30::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<sht30::Data>> _data{};
    config_t _cfg{};
};

///@cond
namespace sht30 {
namespace command {
// Measurement Commands for Single Shot Data Acquisition Mode
constexpr uint16_t SINGLE_SHOT_ENABLE_STRETCH_HIGH{0x2C06};
constexpr uint16_t SINGLE_SHOT_ENABLE_STRETCH_MEDIUM{0x2C0D};
constexpr uint16_t SINGLE_SHOT_ENABLE_STRETCH_LOW{0x2C10};
constexpr uint16_t SINGLE_SHOT_DISABLE_STRETCH_HIGH{0x2400};
constexpr uint16_t SINGLE_SHOT_DISABLE_STRETCH_MEDIUM{0x240B};
constexpr uint16_t SINGLE_SHOT_DISABLE_STRETCH_LOW{0x2416};
// Measurement Commands for Periodic Data Acquisition Mode
constexpr uint16_t START_PERIODIC_MPS_HALF_HIGH{0x2032};
constexpr uint16_t START_PERIODIC_MPS_HALF_MEDIUM{0x2024};
constexpr uint16_t START_PERIODIC_MPS_HALF_LOW{0x202f};

constexpr uint16_t START_PERIODIC_MPS_1_HIGH{0x2130};
constexpr uint16_t START_PERIODIC_MPS_1_MEDIUM{0x2126};
constexpr uint16_t START_PERIODIC_MPS_1_LOW{0x212D};

constexpr uint16_t START_PERIODIC_MPS_2_HIGH{0x2236};
constexpr uint16_t START_PERIODIC_MPS_2_MEDIUM{0x2220};
constexpr uint16_t START_PERIODIC_MPS_2_LOW{0x222B};

constexpr uint16_t START_PERIODIC_MPS_4_HIGH{0x2334};
constexpr uint16_t START_PERIODIC_MPS_4_MEDIUM{0x2322};
constexpr uint16_t START_PERIODIC_MPS_4_LOW{0x2329};

constexpr uint16_t START_PERIODIC_MPS_10_HIGH{0x2737};
constexpr uint16_t START_PERIODIC_MPS_10_MEDIUM{0x2721};
constexpr uint16_t START_PERIODIC_MPS_10_LOW{0x272A};

constexpr uint16_t STOP_PERIODIC_MEASUREMENT{0x3093};
constexpr uint16_t ACCELERATED_RESPONSE_TIME{0x2B32};
constexpr uint16_t READ_MEASUREMENT{0xE000};
// Reset
constexpr uint16_t SOFT_RESET{0x30A2};
// Heater
constexpr uint16_t START_HEATER{0x306D};
constexpr uint16_t STOP_HEATER{0x3066};
// Status
constexpr uint16_t READ_STATUS{0xF32D};
constexpr uint16_t CLEAR_STATUS{0x3041};
// Serial
constexpr uint16_t GET_SERIAL_NUMBER_ENABLE_STRETCH{0x3780};
constexpr uint16_t GET_SERIAL_NUMBER_DISABLE_STRETCH{0x3682};
}  // namespace command
}  // namespace sht30
///@endcond

}  // namespace unit
}  // namespace m5
#endif
