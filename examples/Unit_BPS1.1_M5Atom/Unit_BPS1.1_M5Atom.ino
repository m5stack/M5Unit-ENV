/**
 * @file Unit_BPS1.1_M5Atom.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief 
 * @version 0.1
 * @date 2024-01-30
 *
 *
 * @Hardwares: M5Atom + Unit BPS 1.1
 * @Platform Version: Arduino M5Stack Board Manager v2.1.0
 * @Dependent Library:
 * M5UnitENV: https://github.com/m5stack/M5Unit-ENV
 */

#include "M5UnitENV.h"

QMP6988 qmp;

void setup() {
    Serial.begin(115200);

    if (!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 26, 32, 400000U)) {
        Serial.println("Couldn't find QMP6988");
        while (1) delay(1);
    }
}

void loop() {
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
