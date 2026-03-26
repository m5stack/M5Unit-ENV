# M5Unit - ENV

## Overview

### SKU:U001 & U001-B & U001-C & U090 & U090-B & U053-B & U053-D & U103

Contains M5Stack-**UNIT ENV & Hat ENV & UNIT BPS & UNIT CO2** series related case programs.

Unit ENV is an environmental sensor that integrates DHT12 and BMP280 internally to detect temperature, humidity, and atmospheric pressure data.

Unit ENV II integrates SHT30 and BMP280. Unit ENV III integrates SHT30 and QMP6988.

Unit Mini BPS uses the Bosch BMP280 pressure sensor. Unit Mini BPS v1.1 uses QMP6988 to measure atmospheric pressure and altitude estimation.

Hat ENV II and Hat ENV III are Hat form factor versions of ENV II and ENV III for M5StickC series.

Unit CO2 is a digital air CO2 concentration detection unit, built-in with Sensirion's SCD40 sensor.

## Related Link

- [Unit ENVIV - Document & Datasheet](https://docs.m5stack.com/en/unit/Unit_ENV-IV)
- [Unit ENVIII - Document & Datasheet](https://docs.m5stack.com/en/unit/envIII)
- [Unit ENVII - Document & Datasheet](https://docs.m5stack.com/en/unit/envII)
- [Unit ENV - Document & Datasheet](https://docs.m5stack.com/en/unit/env)
- [Hat ENVIII - Document & Datasheet](https://docs.m5stack.com/en/hat/hat_envIII)
- [Hat ENVII - Document & Datasheet](https://docs.m5stack.com/en/hat/hat_envII)
- [Unit BPS - Document & Datasheet](https://docs.m5stack.com/en/unit/bps)
- [Unit BPS(QMP6988) - Document & Datasheet](https://docs.m5stack.com/en/unit/BPS%28QMP6988%29)
- [Unit CO2 - Document & Datasheet](https://docs.m5stack.com/en/unit/co2)

## Required Libraries:

- [Adafruit_BMP280_Library](https://github.com/adafruit/Adafruit_BMP280_Library)
- [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
- [Sensirion I2C SCD4x](https://github.com/Sensirion/arduino-i2c-scd4x)
- [Sensirion I2C SHT4x](https://github.com/Sensirion/arduino-i2c-sht4x)
- [Sensirion Core](https://github.com/Sensirion/arduino-core)

## License

- [M5Unit-ENV - MIT](LICENSE)

---

## M5UnitUnified
Library for Unit ENV using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### Supported units
- Unit CO2 (SKU:U103)
- Unit CO2L (SKU:U104)
- Unit ENVIII (SKU:U001-C)
- Unit ENVIV (SKU:U001-D)
- Unit ENVPro (SKU:U169)
- Unit TVOC (SKU:U088)
- Hat ENVIII (SKU:U053-D)

### SKU:U088

TVOC/eCO2 mini Unit is a digital multi-pixel gas sensor unit with integrated SGP30.

It mainly measures various VOC (volatile organic compounds) and H2 in the air. It can be programmed to detect TVOC (total volatile organic compounds) and eCO2 (equivalent carbon dioxide reading) Concentration measurement.

Typical measurement accuracy is 15% within the measurement range, the SGP30 reading is internally calibrated and output, which can maintain long-term stability. SGP30 uses I2C protocol communication with on-chip humidity compensation function, which can be turned on through an external humidity sensor.

If you need to obtain accurate results, you need to calibrate according to a known measurement source. SGP30 has a built-in calibration function. In addition, eCO2 is calculated based on the concentration of H2 and cannot completely replace "true" CO2 sensors for laboratory use.

## Related Link
See also examples using conventional methods here.

- [UnitTVOC/eCO2 & Datasheet](https://docs.m5stack.com/en/unit/tvoc)


### Include file
```cpp
#include <M5UnitUnifiedENV.h> // For UnitUnified
//#include <M5UnitENV.h> // When using M5UnitUnified, do not use it at the same time as conventional libraries
```
Supported units will be added in the future.

### Required Libraries:
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

The Bosch library is required by ENVPro to obtain values that cannot be obtained without using the Bosch library.
- [Bosch-BME68x-Library](https://github.com/boschsensortec/Bosch-BME68x-Library)
- [Bosch-BSEC2-Library](https://github.com/boschsensortec/Bosch-BSEC2-Library) (Excluding NanoC6)

### Examples
See also [examples/UnitUnified](examples/UnitUnified)

### Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-ENV/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

#### Required
- [Doxygen](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)
