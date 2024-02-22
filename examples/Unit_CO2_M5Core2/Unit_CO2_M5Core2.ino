/**
 * @file Unit_CO2_M5Core2.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief
 * @version 0.1
 * @date 2024-01-30
 *
 *
 * @Hardwares: M5Core2 + Unit CO2
 * @Platform Version: Arduino M5Stack Board Manager v2.1.0
 * @Dependent Library:
 * M5UnitENV: https://github.com/m5stack/M5Unit-ENV
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#include <M5Unified.h>
#include "M5UnitENV.h"

SCD4X scd4x;

void setup() {
    M5.begin();

    if (!scd4x.begin(&Wire, SCD4X_I2C_ADDR, 32, 33, 400000U)) {
        Serial.println("Couldn't find SCD4X");
        while (1) delay(1);
    }

    uint16_t error;
    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    }

    Serial.println("Waiting for first measurement... (5 sec)");
}

void loop() {
    if (scd4x.update())  // readMeasurement will return true when
                         // fresh data is available
    {
        Serial.println();

        Serial.print(F("CO2(ppm):"));
        Serial.print(scd4x.getCO2());

        Serial.print(F("\tTemperature(C):"));
        Serial.print(scd4x.getTemperature(), 1);

        Serial.print(F("\tHumidity(%RH):"));
        Serial.print(scd4x.getHumidity(), 1);

        Serial.println();
    } else {
        Serial.print(F("."));
    }

    delay(1000);
}
