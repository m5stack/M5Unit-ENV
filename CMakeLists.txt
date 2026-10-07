# ESP-IDF component manifest for M5Unit-ENV.
# Upstream: https://github.com/m5stack/M5Unit-ENV
#
# Sensor unit drivers (SCD4x / SHT3x / SHT4x / QMP6988 / BMP280 / BME688 / SGP30) on top of
# M5UnitUnified. The BME688 raw API comes from the bme68x community component; the optional BSEC2
# (IAQ) prebuilt library is fetched on demand by the in-repo components/bsec2 recipe and linked here
# only on Bosch-supported targets when CONFIG_M5UNIT_ENV_ENABLE_BSEC2=y.
#
# Only the M5UnitUnified API (src/unit/) is built here. The legacy standalone drivers at the top of
# src/ (BMP280/DHT12/I2C_Class/QMP6988/SCD4X/SHT3X/SHT4X/utility) are Arduino-only (their headers
# include <Arduino.h>) and must NOT be compiled under ESP-IDF native.
file(GLOB_RECURSE SRCS "src/unit/*.cpp")

# BSEC2 prebuilt (libalgobsec.a) exists only for esp32/s2/s3/c3. Require the bsec2 recipe component
# on those targets so its Kconfig is always visible in menuconfig (graph-resident) and its include
# dirs / prebuilt lib propagate when enabled. c5/c6/h2/p4 never require it (no blob exists).
set(BSEC2_REQUIRES "")
if(IDF_TARGET STREQUAL "esp32" OR IDF_TARGET STREQUAL "esp32s2" OR
   IDF_TARGET STREQUAL "esp32s3" OR IDF_TARGET STREQUAL "esp32c3")
    set(BSEC2_REQUIRES bsec2)
endif()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified M5Utility bme68x ${BSEC2_REQUIRES}
)
