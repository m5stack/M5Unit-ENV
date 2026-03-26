/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SHT20.hpp
  @brief SHT20 Unit for M5UnitUnified
 */
#ifndef M5_UNIT_ENV_UNIT_SHT20_HPP
#define M5_UNIT_ENV_UNIT_SHT20_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

/*!
  @namespace sht20
  @brief For SHT20
 */
namespace sht20 {

/*!
  @enum Resolution
  @brief Measurement resolution (humidity bits / temperature bits)
 */
enum class Resolution : uint8_t {
    RH12_T14 = 0x00,  //!< RH 12 bit, T 14 bit (default)
    RH8_T12  = 0x01,  //!< RH 8 bit, T 12 bit
    RH10_T13 = 0x80,  //!< RH 10 bit, T 13 bit
    RH11_T11 = 0x81,  //!< RH 11 bit, T 11 bit
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 6> raw{};  //!< RAW data (3 bytes temperature + 3 bytes humidity)
    //! temperature (Celsius)
    inline float temperature() const
    {
        return celsius();
    }
    float celsius() const;     //!< temperature (Celsius)
    float fahrenheit() const;  //!< temperature (Fahrenheit)
    float humidity() const;    //!< humidity (RH)
};

}  // namespace sht20

/*!
  @class UnitSHT20
  @brief Temperature and humidity sensor unit (SHT20)
  @details Sensirion SHT20 — I2C address 0x40, 1-byte commands, CRC-8
*/
class UnitSHT20 : public Component, public PeriodicMeasurementAdapter<UnitSHT20, sht20::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSHT20, 0x40);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Measurement resolution
        sht20::Resolution resolution{sht20::Resolution::RH12_T14};
        //! Start heater on begin?
        bool start_heater{false};
        //! Periodic measurement interval (ms). Clamped to resolution minimum
        uint32_t periodic_interval{1000};
    };

    explicit UnitSHT20(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<sht20::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitSHT20()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configuration */
    inline config_t config() const
    {
        return _cfg;
    }
    //! @brief Set the configuration
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
      @return True if successful
    */
    inline bool startPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitSHT20, sht20::Data>::startPeriodicMeasurement();
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
     */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitSHT20, sht20::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @param[out] d Measured data
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool measureSingleshot(sht20::Data& d);
    ///@}

    ///@name Resolution
    ///@{
    /*!
      @brief Read measurement resolution
      @param[out] res Resolution
      @return True if successful
    */
    bool readResolution(sht20::Resolution& res);
    /*!
      @brief Write measurement resolution
      @param res Resolution
      @return True if successful
    */
    bool writeResolution(const sht20::Resolution res);
    ///@}

    ///@name Heater
    ///@{
    /*!
      @brief Enable on-chip heater
      @return True if successful
    */
    bool startHeater();
    /*!
      @brief Disable on-chip heater
      @return True if successful
    */
    bool stopHeater();
    ///@}

    ///@name Reset
    ///@{
    /*!
      @brief Soft reset
      @return True if successful
    */
    bool softReset();
    ///@}

protected:
    bool start_periodic_measurement();
    bool stop_periodic_measurement();
    bool read_measurement(sht20::Data& d);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitSHT20, sht20::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<sht20::Data>> _data{};
    config_t _cfg{};

private:
    bool read_user_register(uint8_t& reg);
    bool write_user_register(const uint8_t reg);
    bool trigger_measurement(const uint8_t cmd, uint16_t& raw);
};

///@cond
namespace sht20 {
namespace command {
constexpr uint8_t TRIGGER_TEMP_HOLD{0xE3};
constexpr uint8_t TRIGGER_HUMD_HOLD{0xE5};
constexpr uint8_t TRIGGER_TEMP_NOHOLD{0xF3};
constexpr uint8_t TRIGGER_HUMD_NOHOLD{0xF5};
constexpr uint8_t WRITE_USER_REG{0xE6};
constexpr uint8_t READ_USER_REG{0xE7};
constexpr uint8_t SOFT_RESET{0xFE};

constexpr uint8_t REG_RESOLUTION_MASK{0x81};
constexpr uint8_t REG_HEATER_BIT{0x04};
}  // namespace command
}  // namespace sht20
///@endcond

}  // namespace unit
}  // namespace m5
#endif
