#include "BMP280.h"

bool BMP280::begin(TwoWire* wire, uint8_t addr, uint8_t sda, uint8_t scl,
                   long freq) {
    _i2c.begin(wire, sda, scl, freq);
    _addr = addr;
    if (!_i2c.exist(_addr)) {
        return false;
    }
    readCoefficients();
    setSampling();

    return true;
}
bool BMP280::update() {
    readTemperature();
    readPressure();
    readAltitude();

    return true;
}

float BMP280::readTemperature() {
    int32_t var1, var2;

    int32_t adc_T = read24(BMP280_REGISTER_TEMPDATA);
    adc_T >>= 4;

    var1 = ((((adc_T >> 3) - ((int32_t)_bmp280_calib.dig_T1 << 1))) *
            ((int32_t)_bmp280_calib.dig_T2)) >>
           11;

    var2 = (((((adc_T >> 4) - ((int32_t)_bmp280_calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)_bmp280_calib.dig_T1))) >>
             12) *
            ((int32_t)_bmp280_calib.dig_T3)) >>
           14;

    t_fine = var1 + var2;

    float T = (t_fine * 5 + 128) >> 8;
    cTemp   = T / 100;
    return cTemp;
}

float BMP280::readPressure() {
    int64_t var1, var2, p;

    // Must be done first to get the t_fine variable set up

    int32_t adc_P = read24(BMP280_REGISTER_PRESSUREDATA);
    adc_P >>= 4;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)_bmp280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)_bmp280_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)_bmp280_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)_bmp280_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)_bmp280_calib.dig_P2) << 12);
    var1 =
        (((((int64_t)1) << 47) + var1)) * ((int64_t)_bmp280_calib.dig_P1) >> 33;

    if (var1 == 0) {
        return 0;  // avoid exception caused by division by zero
    }
    p    = 1048576 - adc_P;
    p    = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)_bmp280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)_bmp280_calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)_bmp280_calib.dig_P7) << 4);
    pressure = p / 256;
    return pressure;
}

/*!
 * @brief Calculates the approximate altitude using barometric pressure and the
 * supplied sea level hPa as a reference.
 * @param seaLevelhPa
 *        The current hPa at sea level.
 * @return The approximate altitude above sea level in meters.
 */
float BMP280::readAltitude(float seaLevelhPa) {
    float pressure = readPressure();  // in Si units for Pascal
    pressure /= 100;
    altitude = 44330 * (1.0 - pow(pressure / seaLevelhPa, 0.1903));
    return altitude;
}

void BMP280::setSampling(sensor_mode mode, sensor_sampling tempSampling,
                         sensor_sampling pressSampling, sensor_filter filter,
                         standby_duration duration) {
    _measReg.mode   = mode;
    _measReg.osrs_t = tempSampling;
    _measReg.osrs_p = pressSampling;

    _configReg.filter = filter;
    _configReg.t_sb   = duration;

    write8(BMP280_REGISTER_CONFIG, _configReg.get());
    write8(BMP280_REGISTER_CONTROL, _measReg.get());
}

void BMP280::readCoefficients() {
    _bmp280_calib.dig_T1 = read16_LE(BMP280_REGISTER_DIG_T1);
    _bmp280_calib.dig_T2 = readS16_LE(BMP280_REGISTER_DIG_T2);
    _bmp280_calib.dig_T3 = readS16_LE(BMP280_REGISTER_DIG_T3);

    _bmp280_calib.dig_P1 = read16_LE(BMP280_REGISTER_DIG_P1);
    _bmp280_calib.dig_P2 = readS16_LE(BMP280_REGISTER_DIG_P2);
    _bmp280_calib.dig_P3 = readS16_LE(BMP280_REGISTER_DIG_P3);
    _bmp280_calib.dig_P4 = readS16_LE(BMP280_REGISTER_DIG_P4);
    _bmp280_calib.dig_P5 = readS16_LE(BMP280_REGISTER_DIG_P5);
    _bmp280_calib.dig_P6 = readS16_LE(BMP280_REGISTER_DIG_P6);
    _bmp280_calib.dig_P7 = readS16_LE(BMP280_REGISTER_DIG_P7);
    _bmp280_calib.dig_P8 = readS16_LE(BMP280_REGISTER_DIG_P8);
    _bmp280_calib.dig_P9 = readS16_LE(BMP280_REGISTER_DIG_P9);
}

/*!
 * Calculates the pressure at sea level (QNH) from the specified altitude,
 * and atmospheric pressure (QFE).
 * @param  altitude      Altitude in m
 * @param  atmospheric   Atmospheric pressure in hPa
 * @return The approximate pressure in hPa
 */
float BMP280::seaLevelForAltitude(float altitude, float atmospheric) {
    // Equation taken from BMP180 datasheet (page 17):
    // http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

    // Note that using the equation from wikipedia can give bad results
    // at high altitude.  See this thread for more information:
    // http://forums.adafruit.com/viewtopic.php?f=22&t=58064
    return atmospheric / pow(1.0 - (altitude / 44330.0), 5.255);
}

/*!
    @brief  calculates the boiling point  of water by a given pressure
    @param pressure pressure in hPa
    @return temperature in °C
*/

float BMP280::waterBoilingPoint(float pressure) {
    // Magnusformular for calculation of the boiling point of water at a given
    // pressure
    return (234.175 * log(pressure / 6.1078)) /
           (17.08085 - log(pressure / 6.1078));
}

/*!
    @brief  Take a new measurement (only possible in forced mode)
    @return true if successful, otherwise false
 */
bool BMP280::takeForcedMeasurement() {
    // If we are in forced mode, the BME sensor goes back to sleep after each
    // measurement and we need to set it to forced mode once at this point, so
    // it will take the next measurement and then return to sleep again.
    // In normal mode simply does new measurements periodically.
    if (_measReg.mode == MODE_FORCED) {
        // set to forced mode, i.e. "take next measurement"
        write8(BMP280_REGISTER_CONTROL, _measReg.get());
        // wait until measurement has been completed, otherwise we would read
        // the values from the last measurement
        while (read8(BMP280_REGISTER_STATUS) & 0x08) delay(1);
        return true;
    }
    return false;
}

/*!
 *  @brief  Resets the chip via soft reset
 */
void BMP280::reset(void) {
    write8(BMP280_REGISTER_SOFTRESET, MODE_SOFT_RESET_CODE);
}

/*!
    @brief  Gets the most recent sensor event from the hardware status register.
    @return Sensor status as a byte.
 */
uint8_t BMP280::getStatus(void) {
    return read8(BMP280_REGISTER_STATUS);
}

/**************************************************************************/
/*!
    @brief  Writes an 8 bit value over I2C/SPI
*/
/**************************************************************************/
void BMP280::write8(byte reg, byte value) {
    _i2c.writeByte(_addr, reg, value);
}

/*!
 *  @brief  Reads an 8 bit value over I2C/SPI
 *  @param  reg
 *          selected register
 *  @return value from selected register
 */
uint8_t BMP280::read8(byte reg) {
    return _i2c.readByte(_addr, reg);
}

/*!
 *  @brief  Reads a 16 bit value over I2C/SPI
 */
uint16_t BMP280::read16(byte reg) {
    uint8_t buffer[2];
    _i2c.readBytes(_addr, reg, buffer, 2);
    return uint16_t(buffer[0]) << 8 | uint16_t(buffer[1]);
}

uint16_t BMP280::read16_LE(byte reg) {
    uint16_t temp = read16(reg);
    return (temp >> 8) | (temp << 8);
}

/*!
 *   @brief  Reads a signed 16 bit value over I2C/SPI
 */
int16_t BMP280::readS16(byte reg) {
    return (int16_t)read16(reg);
}

int16_t BMP280::readS16_LE(byte reg) {
    return (int16_t)read16_LE(reg);
}

/*!
 *  @brief  Reads a 24 bit value over I2C/SPI
 */
uint32_t BMP280::read24(byte reg) {
    uint8_t buffer[3];
    _i2c.readBytes(_addr, reg, buffer, 3);
    return uint32_t(buffer[0]) << 16 | uint32_t(buffer[1]) << 8 |
           uint32_t(buffer[2]);
}