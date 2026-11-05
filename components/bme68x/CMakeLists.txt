# DL-recipe for the Bosch BME68x-Sensor-API (raw register driver, BSD-3-Clause). No Bosch bytes are
# committed to this repository -- parity with the Arduino build, which pulls the same upstream
# "BME68x Sensor library" via its package manager. The pinned GitHub tag archive is fetched at
# configure time; only the raw C API (bme68x.c) is compiled. The Arduino C++ wrapper
# (bme68xLibrary.*) bundled in the tarball is NOT used under ESP-IDF. Headers are exposed so
# <bme68x/bme68x.h> resolves unchanged on both Arduino and ESP-IDF builds.
#
# Pinned to the same version the Arduino build uses (library.json: BME68x Sensor library >=1.3.40408)
# for raw-API parity across the two build systems. Update URL + SHA256 together if it moves.
set(BME68X_VERSION "1.3.40408")
set(BME68X_URL "https://github.com/boschsensortec/Bosch-BME68x-Library/archive/refs/tags/v${BME68X_VERSION}.tar.gz")
set(BME68X_SHA256 "8aed6d17a52ce0fbfc2b5786b332ce6ee9c2de5f31c2e6c9c3e1859ccf887a31")
set(BME68X_DL "${CMAKE_CURRENT_BINARY_DIR}/bme68x-dl")
set(BME68X_TAR "${CMAKE_CURRENT_BINARY_DIR}/bme68x-${BME68X_VERSION}.tar.gz")
# Include root inside the extracted tree. The GitHub tag archive has a top-level
# Bosch-BME68x-Library-<version>/src/ holding bme68x/ (raw API) and the Arduino wrapper
# (verified for the pinned version).
set(BME68X_ROOT "${BME68X_DL}/Bosch-BME68x-Library-${BME68X_VERSION}/src")

if(NOT EXISTS "${BME68X_ROOT}/bme68x/bme68x.h")
    message(STATUS "BME68x: downloading ${BME68X_URL}")
    file(DOWNLOAD "${BME68X_URL}" "${BME68X_TAR}"
         EXPECTED_HASH SHA256=${BME68X_SHA256}
         SHOW_PROGRESS TLS_VERIFY ON)
    file(MAKE_DIRECTORY "${BME68X_DL}")
    file(ARCHIVE_EXTRACT INPUT "${BME68X_TAR}" DESTINATION "${BME68X_DL}")
endif()

idf_component_register(
    SRCS "${BME68X_ROOT}/bme68x/bme68x.c"
    INCLUDE_DIRS "${BME68X_ROOT}")
