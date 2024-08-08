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
  @enum Oversampling
  @brief Oversampling value
 */
enum class Oversampling : uint8_t {
    Skip,  //!< No measurement
    X1,    //!< 1 time
    X2,    //!< 2 times
    X4,    //!< 4 times
    X8,    //!< 8 times
    X16,   //!< 16 times
    X32,   //!< 32 times
    X64,   //!< 64 times
};

/*!
  @enum PowerMode
  @brief Operation mode setting
 */
enum class PowerMode : uint8_t {
    //! @brief Minimal power consumption, but no measurements are taken
    Sleep = 0,
    //! @brief Energise the circuit for measurement only when measuring
    Force = 1,  // 2 also force mode
    //! @brief Normally energized (periodic measurement)
    Normal = 3,
};

/*!
  @struct CtrlMeasurement
  @brief Accessor for CtrlMeasurement
 */
struct CtrlMeasurement {
    ///@name Getter
    ///@{
    Oversampling oversamplingTemperature() const {
        return static_cast<Oversampling>((value >> 5) & 0x07);
    }
    Oversampling oversamplingPressure() const {
        return static_cast<Oversampling>((value >> 2) & 0x07);
    }
    PowerMode mode() const {
        return static_cast<PowerMode>(value & 0x03);
    }
    ///@}

    ///@name Setter
    ///@{
    void oversamplingTemperature(const Oversampling os) {
        value =
            (value & ~(0x07 << 5)) | ((m5::stl::to_underlying(os) & 0x07) << 5);
    }
    void oversamplingPressure(const Oversampling os) {
        value =
            (value & ~(0x07 << 2)) | ((m5::stl::to_underlying(os) & 0x07) << 2);
    }
    void mode(const PowerMode m) {
        value = (value & ~0x03) | (m5::stl::to_underlying(m) & 0x03);
    }
    ///@}
    uint8_t value{};
};

enum class Filter : uint8_t {
    Off,      //!< Off filter
    Coeff2,   //!< co-efficient 2
    Coeff4,   //!< co-efficient 4
    Coeff8,   //!< co-efficient 8
    Coeff16,  //!< co-efficient 16
    Coeff32,  //!< co-efficient 32
};

/*!
  @struct Status
  @brief Accessor for Status
s */
struct Status {
    //! @brief Device operation status
    inline bool measure() const {
        return value & (1U << 3);
    }
    // @brief the status of OTP data access
    inline bool OTP() const {
        return value & (1U << 0);
    }
    uint8_t value{};
};

/*!
  @enum StandbyTime
  @brief Standby time setting for power mode Normal
  @details Used to calculate periodic measurement interval
 */
enum class StandbyTime {
    Time1ms,    //!< @brief 1 ms
    Time5ms,    //!< @brief 5 ms
    Time50ms,   //!< @brief 50 ms
    Time250ms,  //!< @brief 250 ms
    Time500ms,  //!< @brief 500 ms
    Time1sec,   //!< @brief 1 seconds
    Time2sec,   //!< @brief 2 seconds
    Time4sec,   //!< @brief 4 seconds
};

/*!
  @struct IOSetup
  @brief Accessor for IOSetup
 */
struct IOSetup {
    StandbyTime standby() const {
        return static_cast<StandbyTime>((value >> 5) & 0x07);
    }
    void standby(const StandbyTime s) {
        value =
            (value & ~(0x07 << 5)) | ((m5::stl::to_underlying(s) & 0x07) << 5);
    }
    uint8_t value{};
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
    inline float temperature() const {
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
  @brief Barometric pressure sensor to measure atmospheric pressure and altitude
  estimation
*/
class UnitQMP6988
    : public Component,
      public PeriodicMeasurementAdapter<UnitQMP6988, qmp6988::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitQMP6988, 0x70);

   public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t : Component::config_t {
        //! @brief Start periodic measurement on begin?
        bool start_periodic{true};
        //! @brief pressure oversampling
        qmp6988::Oversampling oversampling_pressure{qmp6988::Oversampling::X8};
        //! @brief temperature oversampling
        qmp6988::Oversampling oversampling_temperature{
            qmp6988::Oversampling::X1};
        //! @brief IIR filter
        qmp6988::Filter filter{qmp6988::Filter::Coeff4};
        //! @brief Periodic measurement interval if start periodic on begin
        qmp6988::StandbyTime standby_time{qmp6988::StandbyTime::Time1sec};
    };

    explicit UnitQMP6988(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr),
          _data{new m5::container::CircularBuffer<qmp6988::Data>(1)} {
        auto ccfg  = component_config();
        ccfg.clock = 400000U;
        component_config(ccfg);
    }
    virtual ~UnitQMP6988() {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
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

    ///@name Measurement data by periodic
    ///@{
    //! @brief Oldest measured temperature (Celsius)
    inline float temperature() const {
        return (!empty() && _osTemp != qmp6988::Oversampling::Skip)
                   ? oldest().temperature()
                   : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured temperature (Celsius)
    inline float celsius() const {
        return (!empty() && _osTemp != qmp6988::Oversampling::Skip)
                   ? oldest().celsius()
                   : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured temperature (Fahrenheit)
    inline float fahrenheit() const {
        return (!empty() && _osTemp != qmp6988::Oversampling::Skip)
                   ? oldest().fahrenheit()
                   : std::numeric_limits<float>::quiet_NaN();
    }
    //! @brief Oldest measured pressure (Pa)
    inline float pressure() const {
        return (!empty() && _osPressure != qmp6988::Oversampling::Skip)
                   ? oldest().pressure()
                   : std::numeric_limits<float>::quiet_NaN();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @details Measuring in the current settings
      @param[out] data Measuerd data
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool measureSingleshot(qmp6988::Data& d);
    /*!
      @brief Measurement single shot
      @details Specify settings and measure
      @param[out] data Measuerd data
      @param ost Oversampling for temperature
      @param osp Oversampling for pressure
      @param f filter
      @return True if successful
      @warning The specified settings are maintained after call
      @warning During periodic detection runs, an error is returned
      @warning If Oversampling::Skip is specified, no measurement is taken
    */
    bool measureSingleshot(qmp6988::Data& d, const qmp6988::Oversampling ost,
                           const qmp6988::Oversampling osp,
                           const qmp6988::Filter& f);
    ///@}

    ///@name Typical use case setup
    ///@{
    /*! @brief For weather monitoring */
    inline bool setWeathermonitoring() {
        return setOversamplings(qmp6988::Oversampling::X2,
                                qmp6988::Oversampling::X1) &&
               setFilterCoeff(qmp6988::Filter::Off);
    }
    //! @brief For drop detection
    bool setDropDetection() {
        return setOversamplings(qmp6988::Oversampling::X4,
                                qmp6988::Oversampling::X1) &&
               setFilterCoeff(qmp6988::Filter::Off);
    }
    //! @brief For elevator detection
    bool setElevatorDetection() {
        return setOversamplings(qmp6988::Oversampling::X8,
                                qmp6988::Oversampling::X1) &&
               setFilterCoeff(qmp6988::Filter::Coeff4);
    }
    //! @brief For stair detection
    bool setStairDetection() {
        return setOversamplings(qmp6988::Oversampling::X16,
                                qmp6988::Oversampling::X2) &&
               setFilterCoeff(qmp6988::Filter::Coeff8);
    }
    //! @brief For indoor navigation
    bool setIndoorNavigation() {
        return setOversamplings(qmp6988::Oversampling::X32,
                                qmp6988::Oversampling::X4) &&
               setFilterCoeff(qmp6988::Filter::Coeff32);
    }
    ///@}

    ///@name Measurement condition
    ///@{
    /*!
      @brief Read the measurement conditions
      @param[out] ost Oversampling for temperature
      @param[out] osp Oversampling for pressure
      @return True if successful
    */
    bool readOversamplings(qmp6988::Oversampling& ost,
                           qmp6988::Oversampling& osp);
    /*!
      @brief Read the power mode
      @param[out] mode PowerMode
      @return True if successful
     */
    bool readPowerMode(qmp6988::PowerMode& mode);
    /*!
      @brief Set the measurement conditions
      @param ost Oversampling for temperature
      @param osp Oversampling for pressure
      @return True if successful
      @warning If Oversampling::Skip is specified, no measurement is taken
      @warning During periodic detection runs, an error is returned
    */
    bool setOversamplings(const qmp6988::Oversampling ost,
                          const qmp6988::Oversampling osp);
    //! @brief Set oversampling for temperature
    bool setOversamplingTemperature(const qmp6988::Oversampling os);
    //! @brief Set oversampling for pressure
    bool setOversamplingPressure(const qmp6988::Oversampling os);
    /*!
      @brief Set power mode
      @param mode PowerMode
      @return True if successful
      @note Power mode state changes affect internal operation
      @note When mode is PowerMode::Normal, it becomes a periodic measurement
      @warning set PowerMode::Sleep/Force to stop periodic measurement
      @warning set PowerMode::Normal to startp periodic measurement
     */
    bool setPowerMode(const qmp6988::PowerMode mode);
    ///@}

    ///@name IIR filter co-efficient setting
    ///@{
    /*!
      @brief Read the IIR filter co-efficient
      @param[out] f filter
      @return True if successful
    */
    bool readFilterCoeff(qmp6988::Filter& f);
    /*!
      @brief Sets the IIR filter co-efficient
      @param f filter
      @return True if successful
      @warning During periodic detection runs, an error is returned
    */
    bool setFilterCoeff(const qmp6988::Filter& f);
    ///@}

    ///@name Interval for periodic measurement
    ///@{
    /*!
      @brief Read the standby time
      @param[out] st standby time
      @return True if successful
     */
    bool readStandbyTime(qmp6988::StandbyTime& st);
    /*!
      @brief Sets the standby time
      @param st standby time
      @return True if successful
      @note The periodic measurement interval is calculated by this value,
      oversampling, and filter settings
      @warning During periodic detection runs, an error is returned
    */
    bool setStandbyTime(const qmp6988::StandbyTime st);
    ///@}

    /*! @brief Software reset */
    bool reset();
    /*! @brief Read the status */
    bool readStatus(qmp6988::Status& s);

   protected:
    ///@note Call via startPeriodicMeasurement/stopPeriodicMeasurement
    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @details Measuring in the current settings
      @return True if successful
    */
    bool start_periodic_measurement();
    /*!
      @brief Start periodic measurement
      @details Specify settings and measure
      @param st standby time (same as interval)
      @param ost Oversampling for temperature
      @param osp Oversampling for pressure
      @param f filter
      @return True if successful
      @warning The specified settings are maintained after call
      @warning If Oversampling::Skip is specified, no measurement is taken
    */
    bool start_periodic_measurement(const qmp6988::StandbyTime st,
                                    const qmp6988::Oversampling ost,
                                    const qmp6988::Oversampling osp,
                                    const qmp6988::Filter& f);
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    bool stop_periodic_measurement();
    ///@}

    bool read_calibration(qmp6988::Calibration& c);
    bool read_measurement_condition(uint8_t& cond);
    bool set_measurement_condition(const uint8_t cond);
    bool read_io_setup(uint8_t& s);
    bool set_io_setup(const uint8_t s);
    bool read_measurement(qmp6988::Data& d);
    bool wait_measurement(const uint32_t timeout = 1000);
    bool is_ready_data();

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitQMP6988,
                                                               qmp6988::Data);

   protected:
    std::unique_ptr<m5::container::CircularBuffer<qmp6988::Data>> _data{};

    qmp6988::Oversampling _osTemp{qmp6988::Oversampling::Skip};
    qmp6988::Oversampling _osPressure{qmp6988::Oversampling::Skip};
    qmp6988::PowerMode _mode{qmp6988::PowerMode::Sleep};
    qmp6988::Filter _filter{qmp6988::Filter::Off};
    qmp6988::StandbyTime _standby{qmp6988::StandbyTime::Time1ms};
    qmp6988::Calibration _calibration{};

    config_t _cfg{};
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

constexpr uint8_t RESET{0xE0};

constexpr uint8_t READ_COMPENSATION_COEFFICIENT{0xA0};  // ~ 0xB8 25 bytes

}  // namespace command
}  // namespace qmp6988
///@endcond

}  // namespace unit
}  // namespace m5
#endif
