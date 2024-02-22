/**
 * @file ENV_II.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief 
 * @version 0.1
 * @date 2024-01-30
 *
 *
 * @Hardwares: M5Core + Unit ENV_II
 * @Platform Version: Arduino M5Stack Board Manager v2.1.0
 * @Dependent Library:
 * M5UnitENV: https://github.com/m5stack/M5Unit-ENV
 */

#include "M5UnitENV.h"

SHT3X sht3x;
BMP280 bmp;

void setup() {
    Serial.begin(115200);

    if (!sht3x.begin(&Wire, SHT3X_I2C_ADDR, 21, 22, 400000U)) {
        Serial.println("Couldn't find SHT3X");
        while (1) delay(1);
    }

    if (!bmp.begin(&Wire, BMP280_I2C_ADDR, 21, 22, 400000U)) {
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
