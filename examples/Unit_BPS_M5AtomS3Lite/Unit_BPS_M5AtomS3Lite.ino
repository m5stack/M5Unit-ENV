/**
 * @file Unit_BPS_M5AtomS3Lite.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief 
 * @version 0.1
 * @date 2024-01-30
 *
 *
 * @Hardwares: M5AtomS3Lite + Unit BPS
 * @Platform Version: Arduino M5Stack Board Manager v2.1.0
 * @Dependent Library:
 * M5UnitENV: https://github.com/m5stack/M5Unit-ENV
 */

#include "M5UnitENV.h"

BMP280 bmp;

void setup() {
    Serial.begin(115200);

    if (!bmp.begin(&Wire, BMP280_I2C_ADDR, 2, 1, 400000U)) {
        Serial.println("Couldn't find BMP280");
        while (1) delay(1);
    }
    /* Default settings from datasheet. */
    bmp.setSampling(BMP280::MODE_NORMAL,     /* Operating Mode. */
                    BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    BMP280::FILTER_X16,      /* Filtering. */
                    BMP280::STANDBY_MS_500); /* Standby time. */
}

void loop() {
    if (bmp.update()) {
        Serial.println("-----BMP280-----");
        Serial.print(F("Temperature: "));
        Serial.print(bmp.cTemp);
        Serial.println(" degrees C");
        Serial.print(F("Pressure: "));
        Serial.print(bmp.pressure);
        Serial.println(" Pa");
        Serial.print(F("Approx altitude: "));
        Serial.print(bmp.altitude);
        Serial.println(" m");
        Serial.println("-------------\r\n");
    }
    delay(1000);
}
