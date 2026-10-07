# ESP-IDF project for the ViaPaHub example (M5Unit-ENV).
# Build: idf.py set-target <esp32|esp32s3|...> && idf.py build
cmake_minimum_required(VERSION 3.16.0)
# Shared ESP-IDF defaults for all UnitUnified examples (CPU / tick / stack / core / flash / PSRAM).
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/../common/sdkconfig.defaults")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# M5 log level (E=1 / W=2 / I=3 / D=4 / V=5) for all components. M5_LOG_LEVEL drives M5Utility logs
# (incl. unit internals); CORE_DEBUG_LEVEL is the fallback and covers M5GFX/M5Unified. NOTE: if
# NDEBUG is defined, M5Utility logs are forced OFF regardless.
idf_build_set_property(COMPILE_OPTIONS "-DM5_LOG_LEVEL=3" "-DCORE_DEBUG_LEVEL=3" APPEND)
project(unitenv-viapahub)
