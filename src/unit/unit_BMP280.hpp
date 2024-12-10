/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_BMP280.hpp
  @brief BMP280 Unit for M5UnitUnified
 */
#ifndef M5_UNIT_ENV_UNIT_BNP280_HPP
#define M5_UNIT_ENV_UNIT_BNP280_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

/*!
  @namespace bmp280
  @brief For BMP280
 */
namespace bmp280 {

/*!
  @enum PowerMode
  @brief Operation mode
 */
enum class PowerMode : uint8_t {
    Sleep,          //!< No measurements are performed
    Forced,         //!< Single measurements are performed
    Normal = 0x03,  //!< Periodic measurements are performed
};

/*!
  @enum Oversampling
  @brief Oversampling factor
 */
enum class Oversampling : uint8_t {
    Skipped,  //!< Skipped (No measurements are performed)
    X1,       //!< x1
    X2,       //!< x2
    X4,       //!< x4
    X8,       //!< x8
    X16,      //!< x16
};

/*!
  @enum OversamplingSetting
  @brief Oversampling Settings
 */
enum class OversamplingSetting : uint8_t {
    UltraLowPower,        //!< 16 bit / 2.62 Pa, 16 bit / 0.0050 C
    LowPower,             //!< 17 bit / 1.31 Pa, 16 bit / 0.0050 C
    StandardResolution,   //!< 18 bit / 0.66 Pa, 16 bit / 0.0050 C
    HighResolution,       //!< 19 bit / 0.33 Pa, 16 bit / 0.0050 C
    UltraHighResolution,  //!< 20 bit / 0.16 Pa, 17 bit / 0.0025 C
};

/*!
  @enum Filter
  @brief Filter setting
 */
enum class Filter : uint8_t {
    Off,      //!< Off filter
    Coeff2,   //!< co-efficient 2
    Coeff4,   //!< co-efficient 4
    Coeff8,   //!< co-efficient 8
    Coeff16,  //!< co-efficient 16
};

/*!
  @enum Standby
  @brief Measurement standby time for power mode Normal
 */
enum class Standby : uint8_t {
    Time0_5ms,   //!< 0.5 ms
    Time62_5ms,  //!< 62.5 ms
    Time125ms,   //!< 125 ms
    Time250ms,   //!< 250 ms
    Time500ms,   //!< 500 ms
    Time1sec,    //!< 1 second
    Time2sec,    //!< 2 seconds
    Time4sec,    //!< 4 seconds
};

/*!
  @enum UseCase
  @brief Preset settings
 */
enum class UseCase : uint8_t {
    LowPower,  //!< Handheld device low-power
    Dynamic,   //!< Handheld device dynamic
    Weather,   //!< Weather monitoring
    Elevator,  //!< Elevator / floor change detection
    Drop,      //!< Drop detection
    Indoor,    //!< Indoor navigation
};

/*!
  @union Trimmming
  @brief Trimming parameter
*/
union Trimming {
    uint8_t value[12 * 2]{};
    struct {
        //
        uint16_t dig_T1;
        int16_t dig_T2;
        int16_t dig_T3;
        //
        uint16_t dig_P1;
        int16_t dig_P2;
        int16_t dig_P3;
        int16_t dig_P4;
        int16_t dig_P5;
        int16_t dig_P6;
        int16_t dig_P7;
        int16_t dig_P8;
        int16_t dig_P9;
        // uint16_t reserved;
    } __attribute__((packed));
};

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    std::array<uint8_t, 6> raw{};  //!< RAW data [0,1,2]:pressure [3,4,5]:temperature
    const Trimming* trimming{};    //!< For calculate

    //! temperature (Celsius)
    inline float temperature() const
    {
        return celsius();
    }
    float celsius() const;     //!< temperature (Celsius)
    float fahrenheit() const;  //!< temperature (Fahrenheit)
    float pressure() const;    //!< pressure (Pa)
};

}  // namespace bmp280

/*!
  @class UnitBMP280
  @brief Pressure and temperature sensor unit
*/
class UnitBMP280 : public Component, public PeriodicMeasurementAdapter<UnitBMP280, bmp280::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitBMP280, 0x76);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Pressure oversampling if start on begin
        bmp280::Oversampling osrs_pressure{bmp280::Oversampling::X16};
        //! Temperature oversampling if start on begin
        bmp280::Oversampling osrs_temperature{bmp280::Oversampling::X2};
        //! Filter if start on begin
        bmp280::Filter filter{bmp280::Filter::Coeff16};
        //! Standby time if start on begin
        bmp280::Standby standby{bmp280::Standby::Time1sec};
    };

    explicit UnitBMP280(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<bmp280::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitBMP280()
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
    //! @brief Oldest measured pressure (Pa)
    inline float pressure() const
    {
        return !empty() ? oldest().pressure() : std::numeric_limits<float>::quiet_NaN();
    }
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @param osrsPressure Oversampling factor for pressure
      @param osrsTemperature Oversampling factor for temperature
      @param filter Filter coeff
      @param st Standby time
      @return True if successful
      @warning Measuring pressure requires measuring temperature
    */
    inline bool startPeriodicMeasurement(const bmp280::Oversampling osrsPressure,
                                         const bmp280::Oversampling osrsTemperature, const bmp280::Filter filter,
                                         const bmp280::Standby st)
    {
        return PeriodicMeasurementAdapter<UnitBMP280, bmp280::Data>::startPeriodicMeasurement(
            osrsPressure, osrsTemperature, filter, st);
    }
    //! @brief Start periodic measurement using current settings
    inline bool startPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitBMP280, bmp280::Data>::startPeriodicMeasurement();
    }

    /*!
      @brief Stop periodic measurement
      @return True if successful
     */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitBMP280, bmp280::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @param[out] data Measuerd data
      @param osrsPressure Oversampling factor for pressure
      @param osrsTemperature Oversampling factor for temperature
      @param filter Filter coeff
      @return True if successful
      @warning During periodic detection runs, an error is returned
      @warning Measuring pressure requires measuring temperature
      @warning Each setting is overwritten
    */
    bool measureSingleshot(bmp280::Data& d, const bmp280::Oversampling osrsPressure,
                           const bmp280::Oversampling osrsTemperature, const bmp280::Filter filter);
    //! @brief Measurement single shot using current settings
    inline bool measureSingleshot(bmp280::Data& d)
    {
        return measure_singleshot(d);
    }
    ///@}

    ///@name Settings
    ///@{
    /*!
      @brief Read the oversampling conditions
      @param[out] osrsPressure Oversampling for pressure
      @param[out] osrsTemperature Oversampling for temperature
      @return True if successful
    */
    bool readOversampling(bmp280::Oversampling& osrsPressure, bmp280::Oversampling& osrsTemperature);
    /*!
      @brief Write the oversampling conditions
      @param osrsPressure Oversampling for pressure
      @param osrsTemperature Oversampling for temperature
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeOversampling(const bmp280::Oversampling osrsPressure, const bmp280::Oversampling osrsTemperature);
    /*!
      @brief Write the oversampling conditions for pressure
      @param osrsPressure Oversampling for pressure
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeOversamplingPressure(const bmp280::Oversampling osrsPressure);
    /*!
      @brief Write the oversampling conditions for temperature
      @param osrsTemperature Oversampling for temperature
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeOversamplingTemperature(const bmp280::Oversampling osrsTemperature);
    /*!
      @brief Write the oversampling by OversamplingSetting
      @param osrss OversamplingSetting
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeOversampling(const bmp280::OversamplingSetting osrss);
    /*!
      @brief Read the IIR filter co-efficient
      @param[out] f filter
      @return True if successful
    */
    bool readFilter(bmp280::Filter& f);
    /*!
      @brief Write the IIR filter co-efficient
      @param f filter
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeFilter(const bmp280::Filter& f);
    /*!
      @brief Read the standby time
      @param[out] s standby time
      @return True if successful
     */
    bool readStandbyTime(bmp280::Standby& s);
    /*!
      @brief Write the standby time
      @param s standby time
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool writeStandbyTime(const bmp280::Standby s);
    /*!
      @brief Read the power mode
      @param[out] m Power mode
      @return True if successful
    */
    bool readPowerMode(bmp280::PowerMode& m);
    /*!
      @brief Write the power mode
      @param m Power mode
      @return True if successful
      @warning Note that the measurement mode is changed
      @warning It is recommended to use start/stopPeriodicMeasurement or similar to change the measurement mode
    */
    bool writePowerMode(const bmp280::PowerMode m);
    /*!
      @brief Write the settings based on use cases
      @param uc UseCase
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool writeUseCaseSetting(const bmp280::UseCase uc);
    ///@}

    /*!
      @brief Soft reset
      @return True if successful
    */
    bool softReset();

protected:
    bool start_periodic_measurement(const bmp280::Oversampling osrsPressure, const bmp280::Oversampling osrsTemperature,
                                    const bmp280::Filter filter, const bmp280::Standby st);
    bool start_periodic_measurement();
    bool stop_periodic_measurement();
    bool read_measurement(bmp280::Data& d);
    bool measure_singleshot(bmp280::Data& d);

    bool read_trimming(bmp280::Trimming& t);
    bool is_data_ready();

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitBMP280, bmp280::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<bmp280::Data>> _data{};
    config_t _cfg{};
    bmp280::Trimming _trimming{};
};

///@cond
namespace bmp280 {
namespace command {

constexpr uint8_t CHIP_ID{0xD0};
// constexpr uint8_t CHIP_VERSION{0xD1};
constexpr uint8_t SOFT_RESET{0xE0};
constexpr uint8_t GET_STATUS{0xF3};
constexpr uint8_t CONTROL_MEASUREMENT{0xF4};
constexpr uint8_t CONFIG{0xF5};

constexpr uint8_t GET_MEASUREMENT{0XF7};       // 6bytes
constexpr uint8_t GET_PRESSURE{0xF7};          // 3byts
constexpr uint8_t GET_PRESSURE_MSB{0xF7};      // 7:0
constexpr uint8_t GET_PRESSURE_LSB{0xF8};      // 7:0
constexpr uint8_t GET_PRESSURE_XLSB{0xF9};     // 7:4
constexpr uint8_t GET_TEMPERATURE{0XFA};       // 3 bytes
constexpr uint8_t GET_TEMPERATURE_MSB{0XFA};   // 7:0
constexpr uint8_t GET_TEMPERATURE_LSB{0XFB};   // 7:0
constexpr uint8_t GET_TEMPERATURE_XLSB{0XFC};  // 7:4

constexpr uint8_t TRIMMING_DIG{0x88};  // 12 bytes
constexpr uint8_t TRIMMING_DIG_T1{0x88};
constexpr uint8_t TRIMMING_DIG_T2{0x8A};
constexpr uint8_t TRIMMING_DIG_T3{0x8C};
constexpr uint8_t TRIMMING_DIG_P1{0x8E};
constexpr uint8_t TRIMMING_DIG_P2{0x90};
constexpr uint8_t TRIMMING_DIG_P3{0x92};
constexpr uint8_t TRIMMING_DIG_P4{0x94};
constexpr uint8_t TRIMMING_DIG_P5{0x96};
constexpr uint8_t TRIMMING_DIG_P6{0x98};
constexpr uint8_t TRIMMING_DIG_P7{0x9A};
constexpr uint8_t TRIMMING_DIG_P8{0x9C};
constexpr uint8_t TRIMMING_DIG_P9{0x9A};
constexpr uint8_t TRIMMING_DIG_RESERVED{0xA0};

}  // namespace command
}  // namespace bmp280
///@endcond

}  // namespace unit
}  // namespace m5

#endif
