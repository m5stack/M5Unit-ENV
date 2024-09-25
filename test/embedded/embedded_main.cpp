/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  main for UnitTest on embedded
*/
#include <gtest/gtest.h>
#include <M5Unified.h>
#include <esp_system.h>

#pragma message "Embedded setup/loop"

#if __has_include(<esp_idf_version.h>)
#include <esp_idf_version.h>
#else  // esp_idf_version.h has been introduced in Arduino 1.0.5 (ESP-IDF3.3)
#define ESP_IDF_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define ESP_IDF_VERSION                          ESP_IDF_VERSION_VAL(3, 2, 0)
#endif

namespace {
auto& lcd = M5.Display;
}  // namespace

void test()
{
    lcd.fillRect(0, 0, lcd.width() >> 1, lcd.height(), RUN_ALL_TESTS() ? TFT_RED : TFT_GREEN);
}

void setup()
{
    delay(1500);

    M5.begin();

    M5_LOGI("CPP %ld", __cplusplus);
    M5_LOGI("ESP-IDF Version %d.%d.%d", (ESP_IDF_VERSION >> 16) & 0xFF, (ESP_IDF_VERSION >> 8) & 0xFF,
            ESP_IDF_VERSION & 0xFF);
    M5_LOGI("BOARD:%X", M5.getBoard());
    M5_LOGI("Heap: %u", esp_get_free_heap_size());

    lcd.clear(TFT_DARKGRAY);
    ::testing::InitGoogleTest();

#ifdef GTEST_FILTER
    ::testing::GTEST_FLAG(filter) = GTEST_FILTER;
#endif
}

void loop()
{
    test();
#if 0
    delay(1000);
    esp_restart();
#endif
    while (true) {
        delay(10000);
    }
}
