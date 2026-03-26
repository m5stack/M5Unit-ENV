/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_HatYun.hpp
  @brief Hat Yun for M5UnitUnified
*/
#ifndef M5_UNIT_ENV_UNIT_HATYUN_HPP
#define M5_UNIT_ENV_UNIT_HATYUN_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include "unit_SHT20.hpp"
#include "unit_BMP280.hpp"

namespace m5 {
namespace unit {

/*!
  @namespace hatyun
  @brief For HatYun
 */
namespace hatyun {
constexpr uint8_t NUM_LEDS{14};  //!< Number of RGB LEDs on Hat Yun

/*!
  @struct Data
  @brief Light sensor measurement data
 */
struct Data {
    uint16_t light{};  //!< Raw light sensor value
};

}  // namespace hatyun

/*!
  @class HatYun
  @brief Hat Yun is an environmental sensor hat that integrates SHT20, BMP280, and STM32 LED/Light controller
  @details Holds SHT20 (temp/humidity) and BMP280 (pressure) as child components.
  The STM32-based LED/light controller (I2C 0x38) is accessed directly by this class.
  Supported boards: M5StickC / M5StickCPlus / M5StickCPlus2
 */
class HatYun : public Component, public PeriodicMeasurementAdapter<HatYun, hatyun::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(HatYun, 0x38);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Periodic measurement interval (ms)
        uint32_t periodic_interval{1000};
    };

    UnitSHT20 sht20;    //!< @brief SHT20 instance (temperature/humidity)
    UnitBMP280 bmp280;  //!< @brief BMP280 instance (pressure)

    explicit HatYun(const uint8_t addr = DEFAULT_ADDRESS);
    virtual ~HatYun()
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
    //! @brief Oldest light sensor value
    inline uint16_t light() const
    {
        return !empty() ? oldest().light : 0;
    }
    ///@}

    ///@name Periodic measurement (light sensor polling)
    ///@{
    /*!
      @brief Start periodic light sensor polling
      @param interval Polling interval in ms (default 1000)
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const uint32_t interval = 1000)
    {
        return PeriodicMeasurementAdapter<HatYun, hatyun::Data>::startPeriodicMeasurement(interval);
    }
    /*!
      @brief Stop periodic light sensor polling
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<HatYun, hatyun::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@note LED heat affects SHT20/BMP280 temperature readings due to the compact Hat form factor.
    ///  Consider lowering brightness or turning off LEDs when accurate temperature measurement is required.
    ///@name LED control
    ///@{
    /*!
      @brief Write a single LED color
      @param num LED index (0-13)
      @param r Red (0-255)
      @param g Green (0-255)
      @param b Blue (0-255)
      @return True if successful
    */
    bool writeLED(const uint8_t num, const uint8_t r, const uint8_t g, const uint8_t b);
    /*!
      @brief Write all LEDs to the same color
      @param r Red (0-255)
      @param g Green (0-255)
      @param b Blue (0-255)
      @return True if successful
    */
    bool writeAllLED(const uint8_t r, const uint8_t g, const uint8_t b);
    /*!
      @brief Write rainbow pattern across all LEDs
      @param offset Hue offset (0-255) for animation
      @param brightness Brightness (0-255)
      @return True if successful
    */
    bool writeRainbow(const uint8_t offset = 0, const uint8_t brightness = 255);
    ///@}

    ///@name Light sensor
    ///@{
    /*!
      @brief Read light sensor value
      @param[out] value Light sensor value (16-bit)
      @return True if successful
    */
    bool readLight(uint16_t& value);
    ///@}

protected:
    bool start_periodic_measurement(const uint32_t interval);
    bool stop_periodic_measurement();
    bool read_measurement(hatyun::Data& d);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(HatYun, hatyun::Data);

    virtual std::shared_ptr<Adapter> ensure_adapter(const uint8_t ch);

protected:
    std::unique_ptr<m5::container::CircularBuffer<hatyun::Data>> _data{};
    config_t _cfg{};

private:
    bool _valid{};  // Did the constructor correctly add the child unit?
    Component* _children[2]{&sht20, &bmp280};
};

///@cond
namespace hatyun {
namespace command {
constexpr uint8_t LIGHT_REG{0x00};
constexpr uint8_t LED_REG{0x01};
}  // namespace command
}  // namespace hatyun
///@endcond

}  // namespace unit
}  // namespace m5
#endif
