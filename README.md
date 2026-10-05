# M5Unit - ENV

## Overview

Library for Unit ENV using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

For information on legacy libraries, [see below](#legacy-library).

### SKU:U103
Unit CO2 is a digital air CO2 concentration detection unit, built-in with Sensirion's SCD40 sensor and power step-down circuit, using I2C communication.

### SKU:U104
Unit CO2L is a digital air CO2 concentration detection unit with a low power consumption mode for single measurements.

### SKU:U001-C
Unit ENV-III is an environmental sensor that integrates SHT30 and QMP6988 internally to detect temperature, humidity, and atmospheric pressure data.

### SKU:U001-D
Unit ENV-IV is an environmental sensor unit embedded with SHT40 and BMP280 sensors for measuring temperature, humidity, and atmospheric pressure data.

### SKU:U169
The Unit ENV-Pro sensor is a highly integrated environmental detection unit, equipped with the BME688 sensor solution.

### SKU:U088
The Unit Mini TVOC/eCO2 is a digital multi-pixel gas sensor unit with an integrated SGP30, primarily designed to measure various VOCs (Volatile Organic Compounds) and H2 in the air.

### SKU:U090
Unit Mini BPS is a barometer unit that uses the Bosch BMP280 pressure sensor for measuring atmospheric pressure and altitude estimation.

### SKU:U090-B
Unit Mini BPS v1.1 is a digital barometric pressure sensor unit that uses the QMP6988 barometric pressure sensor and utilizes an I2C communication interface.

### SKU:U053-D
Hat ENV-III is a multifunctional environmental sensor compatible with the M5StickC series. It integrates SHT30 and QMP6988 to detect temperature, humidity, and atmospheric pressure data.

### SKU:U070
Hat Yun is a cloud-shaped multifunctional environmental information collection base. It is equipped with a temperature and humidity sensor SHT20, a pressure sensor BMP280, a photoresistor, and 14 RGB LEDs.

## Related Link
- [Unit CO2 - Document & Datasheet](https://docs.m5stack.com/en/unit/co2)
- [Unit CO2L - Document & Datasheet](https://docs.m5stack.com/en/unit/CO2L)
- [Unit ENVIII - Document & Datasheet](https://docs.m5stack.com/en/unit/envIII)
- [Unit ENVIV - Document & Datasheet](https://docs.m5stack.com/en/unit/Unit_ENV-IV)
- [Unit ENVPro - Document & Datasheet](https://docs.m5stack.com/en/unit/ENV%20Pro%20Unit)
- [UnitTVOC/eCO2 & Datasheet](https://docs.m5stack.com/en/unit/tvoc)
- [Unit BPS - Document & Datasheet](https://docs.m5stack.com/en/unit/bps)
- [Unit BPS(QMP6988) - Document & Datasheet](https://docs.m5stack.com/en/unit/BPS%28QMP6988%29)
- [Hat ENVIII - Document & Datasheet](https://docs.m5stack.com/en/hat/hat_envIII)
- [Hat Yun - Document & Datasheet](https://docs.m5stack.com/en/hat/hat-yun)


## Required Libraries
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

ENVPro (BME688) additionally needs the Bosch libraries:
- [Bosch-BME68x-Library](https://github.com/boschsensortec/Bosch-BME68x-Library) — raw temperature / pressure / humidity / gas (all targets)
- [Bosch-BSEC2-Library](https://github.com/boschsensortec/Bosch-BSEC2-Library) — air-quality (IAQ) output, optional; install it only when using BME688 IAQ. On Arduino / PlatformIO, IAQ is enabled on ESP32 / ESP32-S3 (excluded on ESP32-C6 [NanoC6 / NessoN1], ESP32-H2 [NanoH2], ESP32-P4 [Tab5], where no Bosch prebuilt exists)

On Arduino / PlatformIO, BME68x is installed as a library dependency. BSEC2 is not a required dependency; add `boschsensortec/bsec2` to your application `lib_deps` or install it with Arduino IDE Library Manager only when you use UnitENVPro IAQ APIs. On ESP-IDF native, BME68x is fetched at build time and BSEC2 is fetched only when you opt in for the ENVPro example (see [For ESP-IDF settings](#for-esp-idf-settings)).

### Enabling BSEC2 IAQ

UnitENVPro works without BSEC2 and reports raw temperature, pressure, humidity, and gas resistance. BSEC2 adds Bosch air-quality (IAQ) APIs such as `iaq()` and `bsec2Version()`, and is **opt-in**. The Bosch prebuilt library is required, so BSEC2 is unavailable on ESP32-C6 / ESP32-H2 / ESP32-P4 (NanoC6 / NessoN1 / NanoH2 / Tab5); on those targets ENVPro reports raw measurements only. Enabling BSEC2 downloads the Bosch package at build time and **constitutes acceptance of the Bosch BSEC license**; no Bosch binaries are stored in this repository.

**Arduino / PlatformIO** — install the Bosch BSEC2 library explicitly on a supported target (ESP32 / ESP32-S3):

```ini
lib_deps =
  m5stack/M5Unit-ENV
  boschsensortec/bsec2
```

This repository also provides opt-in PlatformIO environments for checking the IAQ build path:

```sh
pio run -e UnitENVPro_BSEC2_PlotToSerial_Core_Arduino_latest
pio test -e test_BME688_BSEC2_Core
```

Equivalent `_BSEC2` environments are provided for every Arduino board environment in this repository that targets a Bosch-supported ESP32 / ESP32-S3 chip. For Arduino IDE, install **bsec2** from Library Manager before compiling a sketch that uses BSEC2-only APIs.

**ESP-IDF (`idf.py`)** — BSEC2 is enabled via Kconfig (`menuconfig`) instead of installing a library, and is offered on esp32 / esp32-s3 / esp32-c3:

```sh
cd examples/UnitUnified/UnitENVPro/PlotToSerial
idf.py set-target esp32          # esp32 / esp32s2 / esp32s3
idf.py menuconfig
# -> M5Unit-ENV ENVPro example (BSEC2 / BME688 IAQ) -> [*] Enable BSEC2 (IAQ) for BME688
idf.py build flash monitor
```

The **`M5Unit-ENV ENVPro example (BSEC2 / BME688 IAQ)`** menu item appears only on esp32 / esp32-s3 / esp32-c3; on esp32-c6 / esp32-h2 / esp32-p4 the Bosch prebuilt does not exist, so the item is hidden. Default is off (raw measurements only).


## License

- [M5Unit-ENV - MIT](LICENSE)


## Examples
See also [examples/UnitUnified](examples/UnitUnified)

### For ArduinoIDE settings
The UnitENVIII example supports both Unit and Hat variants. Select the variant by uncommenting the appropriate `#define` at the top of the example sketch.

```cpp
#if !defined(USING_UNIT_ENV3) && !defined(USING_HAT_ENV3)
// For Unit ENVIII (U001-C)
// #define USING_UNIT_ENV3
// For Hat ENVIII (U053-D)
// #define USING_HAT_ENV3
#endif
```

### For ESP-IDF settings

> **NOTE:** The ESP-IDF native build (`idf.py`) targets ESP-IDF **5.1 or later** (5.x and 6.x).  
> Hat ENVIII on NessoN1 uses its HatPort, which is the ESP32-C6 LP I2C. This requires ESP-IDF **5.3 or later** for the ESP-IDF native build, and Arduino-ESP32 **3.2 or later** for Arduino.

On ESP-IDF native builds (`idf.py`), options are selected via Kconfig (`menuconfig`) instead of editing the source `#define`.

**UnitENVIII variant (Unit / Hat)** — the example offers the same choice as the Arduino build through `main/Kconfig.projbuild` -> `examples/UnitUnified/common/Kconfig.variant`; `variant.cmake` maps the chosen `CONFIG_EXAMPLE_USING_*` to the source-level `USING_*` macro:

```sh
cd examples/UnitUnified/UnitENVIII/PlotToSerial
idf.py set-target esp32          # or esp32s3 / esp32c6 / esp32h2 / esp32p4
idf.py menuconfig
# -> M5Unit-ENV ENVIII example -> Target unit / board -> UnitENV3 / HatENV3
idf.py build flash monitor
```

**UnitENVPro example (BSEC2 / BME688 IAQ)** — ENVPro can additionally output Bosch BSEC2 air-quality (IAQ) values via `menuconfig` on esp32 / esp32-s3 / esp32-c3. See [Enabling BSEC2 IAQ](#enabling-bsec2-iaq) for the full opt-in steps (Arduino / PlatformIO and ESP-IDF).

## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-ENV/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [Git](https://git-scm.com/) (Output commit hash to html)



---
## Legacy library

The legacy library provides standalone sensor drivers that do not depend on M5UnitUnified. When using M5UnitUnified, do not use it at the same time as the legacy library.

Contains M5Stack-**UNIT ENV & Hat ENV & UNIT BPS & UNIT CO2** series related case programs.

Unit ENV is an environmental sensor that integrates DHT12 and BMP280 internally to detect temperature, humidity, and atmospheric pressure data.

Unit ENV II integrates SHT30 and BMP280. Unit ENV III integrates SHT30 and QMP6988.

Unit Mini BPS uses the Bosch BMP280 pressure sensor. Unit Mini BPS v1.1 uses QMP6988 to measure atmospheric pressure and altitude estimation.

Hat ENV II and Hat ENV III are Hat form factor versions of ENV II and ENV III for M5StickC series.

Unit CO2 is a digital air CO2 concentration detection unit, built-in with Sensirion's SCD40 sensor.

### Related Link
- [Unit ENV - Document & Datasheet](https://docs.m5stack.com/en/unit/env)
- [Unit ENVII - Document & Datasheet](https://docs.m5stack.com/en/unit/envII)
- [Hat ENVII - Document & Datasheet](https://docs.m5stack.com/en/hat/hat_envII)

### Include file
```cpp
#include <M5UnitENV.h>
```

### Required Libraries

- [Adafruit_BMP280_Library](https://github.com/adafruit/Adafruit_BMP280_Library)
- [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
- [Sensirion I2C SCD4x](https://github.com/Sensirion/arduino-i2c-scd4x)
- [Sensirion I2C SHT4x](https://github.com/Sensirion/arduino-i2c-sht4x)
- [Sensirion Core](https://github.com/Sensirion/arduino-core)
