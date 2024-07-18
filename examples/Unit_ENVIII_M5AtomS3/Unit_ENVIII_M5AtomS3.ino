/**
 * @file ENV_III.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief
 * @version 0.2
 * @date 2024-07-18
 *
 *
 * @Hardwares: M5AtomS3 + Unit ENV_III
 * @Platform Version: Arduino M5Stack Board Manager v2.1.0
 * @Dependent Library:
 * M5UnitENV: https://github.com/m5stack/M5Unit-ENV
 */

#include "M5UnitENV.h"

SHT3X sht3x;
QMP6988 qmp;

void setup() {
    Serial.begin(115200);
    if (!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 2, 1, 400000U)) {
        while (1) {
            Serial.println("Couldn't find QMP6988");
            delay(500);
        }
    }

    if (!sht3x.begin(&Wire, SHT3X_I2C_ADDR, 2, 1, 400000U)) {
        while (1) {
            Serial.println("Couldn't find SHT3X");
            delay(500);
        }
    }
}

void loop() {
    if (sht3x.update()) {
        Serial.println("-----SHT3X-----");
        Serial.print("Temperature: ");
        Serial.print(sht3x.cTemp);
        Serial.println(" degrees C");
        Serial.print("Humidity: ");
        Serial.print(sht3x.humidity);
        Serial.println("% rH");
        Serial.println("-------------\r\n");
    }

    if (qmp.update()) {
        Serial.println("-----QMP6988-----");
        Serial.print(F("Temperature: "));
        Serial.print(qmp.cTemp);
        Serial.println(" *C");
        Serial.print(F("Pressure: "));
        Serial.print(qmp.pressure);
        Serial.println(" Pa");
        Serial.print(F("Approx altitude: "));
        Serial.print(qmp.altitude);
        Serial.println(" m");
        Serial.println("-------------\r\n");
    }
    delay(1000);
}
