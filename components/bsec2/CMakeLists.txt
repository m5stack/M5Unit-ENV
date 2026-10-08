# DL-recipe for the Bosch BSEC2 prebuilt library. No Bosch bytes are committed to this repository.
# OFF (default): registers an empty component (no include dirs, no link) -> __has_include false in
# unit_BME688.hpp -> raw measurements only. ON: downloads the pinned upstream package, exposes its
# nested include root (so <inc/...> and <config/...> resolve unchanged), and links the prebuilt
# libalgobsec.a for the current target. Enabling = acceptance of the Bosch BSEC license.

set(BSEC2_INCLUDE_DIRS "")

if(CONFIG_M5UNIT_ENV_ENABLE_BSEC2)
    # Pinned upstream package (verified URL + SHA256). Update both together if it moves.
    set(BSEC2_VERSION "1.8.2610")
    set(BSEC2_URL "https://github.com/boschsensortec/Bosch-BSEC2-Library/archive/refs/tags/v${BSEC2_VERSION}.tar.gz")
    set(BSEC2_SHA256 "85c546700d56549b26e01e4e28db2a9e04d7c11fd9aa40b4fc44f7dae54a962f")
    set(BSEC2_DL "${CMAKE_CURRENT_BINARY_DIR}/bsec2-dl")
    set(BSEC2_TAR "${CMAKE_CURRENT_BINARY_DIR}/bsec2-${BSEC2_VERSION}.tar.gz")
    # Include root inside the extracted tree. The GitHub tag archive has a top-level
    # Bosch-BSEC2-Library-<version>/src/ holding inc/, config/, and <target>/libalgobsec.a
    # (verified for the pinned version).
    set(BSEC2_ROOT "${BSEC2_DL}/Bosch-BSEC2-Library-${BSEC2_VERSION}/src")

    if(NOT EXISTS "${BSEC2_ROOT}/inc/bsec_interface.h")
        message(STATUS "BSEC2: downloading ${BSEC2_URL}")
        file(DOWNLOAD "${BSEC2_URL}" "${BSEC2_TAR}"
             EXPECTED_HASH SHA256=${BSEC2_SHA256}
             SHOW_PROGRESS TLS_VERIFY ON)
        file(MAKE_DIRECTORY "${BSEC2_DL}")
        file(ARCHIVE_EXTRACT INPUT "${BSEC2_TAR}" DESTINATION "${BSEC2_DL}")
    endif()

    if(NOT EXISTS "${BSEC2_ROOT}/${IDF_TARGET}/libalgobsec.a")
        message(FATAL_ERROR "BSEC2: no prebuilt libalgobsec.a for target ${IDF_TARGET}")
    endif()

    set(BSEC2_INCLUDE_DIRS "${BSEC2_ROOT}")
endif()

idf_component_register(INCLUDE_DIRS ${BSEC2_INCLUDE_DIRS})

if(CONFIG_M5UNIT_ENV_ENABLE_BSEC2)
    add_prebuilt_library(algobsec "${BSEC2_ROOT}/${IDF_TARGET}/libalgobsec.a")
    # This component has no SRCS, so COMPONENT_LIB is an INTERFACE library -> link with INTERFACE
    # (not PUBLIC). The dependency propagates to M5Unit-ENV via its REQUIRES bsec2.
    target_link_libraries(${COMPONENT_LIB} INTERFACE algobsec)
endif()
