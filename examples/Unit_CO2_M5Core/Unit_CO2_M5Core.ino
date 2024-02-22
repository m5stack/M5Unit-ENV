
#include "M5UnitENV.h"

SCD4X scd4x;

void setup() {
    Serial.begin(115200);

    if (!scd4x.begin(&Wire, SCD4X_I2C_ADDR, 21, 22, 400000U)) {
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
