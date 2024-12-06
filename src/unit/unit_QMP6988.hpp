/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_QMP6988.hpp
  @brief QMP6988 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_ENV_UNIT_QMP6988_HPP
#define M5_UNIT_ENV_UNIT_QMP6988_HPP
#include <M5UnitComponent.hpp>
#include <m5_utility/stl/extension.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <limits>  // NaN

namespace m5 {
namespace unit {

namespace qmp6988 {

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
  @brief Oversampling value
  @warning
 */
enum class Oversampling : uint8_t {
    Skipped,  //!< Skipped (No measurements are performed)
    X1,       //!< x1
    X2,       //!< x2
    X4,       //!< x4
    X8,       //!< x8
    X16,      //!< x16
    X32,      //!< x32
    X64,      //!< x64
};

/*!
  @enum OversamplingSetting
  @brief Oversampling Settings
 */
enum class OversamplingSetting : uint8_t {
    HighSpeed,           //!< osrsP:X2 osrsT:X1
    LowPower,            //!< osrsP:X4 osrsT:X1
    Standard,            //!< osrsP:X8 osrsT:X1
    HighAccuracy,        //!< osrsP:X16 osrsT:X2
    UltraHightAccuracy,  //!< osrsP:X32 osrsT:X4
};

/*!
  @enum Filter
  @brief Filtter setting
 */
enum class Filter : uint8_t {
    Off,      //!< Off filter
    Coeff2,   //!< co-efficient 2
    Coeff4,   //!< co-efficient 4
    Coeff8,   //!< co-efficient 8
    Coeff16,  //!< co-efficient 16
    Coeff32,  //!< co-efficient 32
};

/*!
  @enum Standby
  @brief Measurement standby time for power mode Normal
 */
enum class Standby : uint8_t {
    Time1ms,    //!< 1 ms
    Time5ms,    //!< 5 ms
    Time50ms,   //!< 50 ms
    Time250ms,  //!< 250 ms
    Time500ms,  //!< 500 ms
    Time1sec,   //!< 1 seconds
    Time2sec,   //!< 2 seconds
    Time4sec,   //!< 4 seconds
};

/*!
  @enum UseCase
  @brief Preset settings
 */
enum class UseCase : uint8_t {
    Weather,   //!< Weather monitoring
    Drop,      //!< Drop detection
    Elevator,  //!< Elevator / floor change detection
    Stair,     //!< Stair detection
    Indoor,    //!< Indoor navigation
};

///@cond
struct Calibration {
    int32_t b00{}, bt1{}, bp1{};
    int64_t bt2{};
    int32_t b11{}, bp2{}, b12{}, b21{}, bp3{}, a0{}, a1{}, a2{};
};
///@endcond

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
    float pressure() const;    //!< pressure (Pa)
    const Calibration* calib{};
};

};  // namespace qmp6988

/*!
  @class UnitQMP6988
  @brief Barometric pressure sensor to measure atmospheric pressure and altitude estimation
*/
class UnitQMP6988 : public Component, public PeriodicMeasurementAdapter<UnitQMP6988, qmp6988::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitQMP6988, 0x70);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! pressure oversampling if start on begin
        qmp6988::Oversampling osrs_pressure{qmp6988::Oversampling::X8};
        //! temperature oversampling if start on begin
        qmp6988::Oversampling osrs_temperature{qmp6988::Oversampling::X1};
        //! Filter if start on begin
        qmp6988::Filter filter{qmp6988::Filter::Coeff4};
        //! Standby time if start on begin
        qmp6988::Standby standby{qmp6988::Standby::Time1sec};
    };

    explicit UnitQMP6988(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<qmp6988::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 400 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitQMP6988()
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
    inline bool startPeriodicMeasurement(const qmp6988::Oversampling osrsPressure,
                                         const qmp6988::Oversampling osrsTemperature, const qmp6988::Filter f,
                                         const qmp6988::Standby st)
    {
        return PeriodicMeasurementAdapter<UnitQMP6988, qmp6988::Data>::startPeriodicMeasurement(osrsPressure,
                                                                                                osrsTemperature, f, st);
    }
    //! @brief Start periodic measurement using current settings
    inline bool startPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitQMP6988, qmp6988::Data>::startPeriodicMeasurement();
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitQMP6988, qmp6988::Data>::stopPeriodicMeasurement();
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
    bool measureSingleshot(qmp6988::Data& d, const qmp6988::Oversampling osrsPressure,
                           const qmp6988::Oversampling osrsTemperature, const qmp6988::Filter f);
    //! @brief Measurement single shot using current settings
    bool measureSingleshot(qmp6988::Data& d);
    ///@}

    ///@name Settings
    ///@{
    /*!
      @brief Read the oversampling conditions
      @param[out] osrsPressure Oversampling for pressure
      @param[out] osrsTemperature Oversampling for temperature
      @return True if successful
    */
    bool readOversampling(qmp6988::Oversampling& osrsPressure, qmp6988::Oversampling& osrsTemperature);
    /*!
      @brief Write the oversampling conditions
      @param osrsPressure Oversampling for pressure
      @param osrsTemperature Oversampling for temperature
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeOversampling(const qmp6988::Oversampling osrsPressure, const qmp6988::Oversampling osrsTemperature);
    /*!
      @brief Write the oversampling conditions for pressure
      @param osrsPressure Oversampling for pressure
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeOversamplingPressure(const qmp6988::Oversampling osrsPressure);
    /*!
      @brief Write the oversampling conditions for temperature
      @param osrsTemperature Oversampling for temperature
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeOversamplingTemperature(const qmp6988::Oversampling osrsTemperature);
    /*!
      @brief Write the oversampling by OversamplingSetting
      @param osrss OversamplingSetting
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeOversampling(const qmp6988::OversamplingSetting osrss);
    /*!
      @brief Read the IIR filter co-efficient
      @param[out] f filter
      @return True if successful
    */
    bool readFilter(qmp6988::Filter& f);
    /*!
      @brief Write the IIR filter co-efficient
      @param f filter
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeFilter(const qmp6988::Filter f);
    /*!
      @brief Read the standby time
      @param[out] st standby time
      @return True if successful
     */
    bool readStandbyTime(qmp6988::Standby& st);
    /*!
      @brief Write the standby time
      @param st standby time
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool writeStandbyTime(const qmp6988::Standby st);
    /*!
      @brief Read the power mode
      @param[out] mode PowerMode
      @return True if successful
     */
    bool readPowerMode(qmp6988::PowerMode& mode);
    /*!
      @brief Write the power mode
      @param m Power mode
      @return True if successful
      @warning Note that the measurement mode is changed
      @warning It is recommended to use start/stopPeriodicMeasurement or similar to change the measurement mode
    */
    bool writePowerMode(const qmp6988::PowerMode mode);
    /*!
      @brief Write the settings based on use cases
      @param uc UseCase
      @return True if successful
      @warning During periodic detection runs, an error is returned
     */
    bool writeUseCaseSetting(const qmp6988::UseCase uc);
    ///@}

    /*!
      @brief Soft reset
      @return True if successful
    */
    bool softReset();

protected:
    bool start_periodic_measurement();
    bool start_periodic_measurement(const qmp6988::Oversampling ost, const qmp6988::Oversampling osp,
                                    const qmp6988::Filter f, const qmp6988::Standby st);
    bool stop_periodic_measurement();

    bool read_calibration(qmp6988::Calibration& c);

    bool read_measurement(qmp6988::Data& d, const bool only_temperature = false);
    bool is_data_ready();

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitQMP6988, qmp6988::Data);

protected:
    std::unique_ptr<m5::container::CircularBuffer<qmp6988::Data>> _data{};
    qmp6988::Calibration _calibration{};
    config_t _cfg{};
    bool _only_temperature{};
};

///@cond
namespace qmp6988 {
namespace command {

constexpr uint8_t CHIP_ID{0xD1};

constexpr uint8_t READ_PRESSURE{0xF7};     // ~ F9 3bytes
constexpr uint8_t READ_TEMPERATURE{0xFA};  // ~ FC 3bytes

constexpr uint8_t IO_SETUP{0xF5};
constexpr uint8_t CONTROL_MEASUREMENT{0xF4};
constexpr uint8_t GET_STATUS{0xF3};
constexpr uint8_t IIR_FILTER{0xF1};

constexpr uint8_t SOFT_RESET{0xE0};

constexpr uint8_t READ_COMPENSATION_COEFFICIENT{0xA0};  // ~ 0xB8 25 bytes

}  // namespace command
}  // namespace qmp6988
///@endcond

}  // namespace unit
}  // namespace m5
#endif
