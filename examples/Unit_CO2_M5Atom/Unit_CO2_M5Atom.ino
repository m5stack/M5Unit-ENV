/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with Atom-Lite/Matrix sample source code
*                          配套  Atom-Lite/Matrix 示例源代码
* Visit for more information: https://docs.m5stack.com/en/unit/co2
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/co2
*
* Product: CO2.  二氧化碳
* Date: 2022/9/1
******************************************** ***********************************
  Please connect Unit to the port, Read CO2 content, temperature, humidity and
  display them on the display screen
  请Unit连接至端口,读取二氧化碳含量,温度,适度,并显示在屏幕上
  Libraries:
  - [Sensirion I2C SCD4x](https://github.com/Sensirion/arduino-i2c-scd4x)]
  - [Sensirion Core](https://github.com/Sensirion/arduino-core)
*/

#include <M5Atom.h>
#include <SensirionI2CScd4x.h>

SensirionI2CScd4x scd4x;

void setup() {
    M5.begin(true, false, true);
    Wire.begin(26, 32);
    uint16_t error;
    char errorMessage[256];
    M5.dis.fillpix(0x00ffff);
    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    Serial.println("Waiting for first measurement... (5 sec)");
}

void loop() {
    uint16_t error;
    char errorMessage[256];

    delay(100);

    // Read Measurement
    uint16_t co2      = 0;
    float temperature = 0.0f;
    float humidity    = 0.0f;
    bool isDataReady  = false;
    error             = scd4x.getDataReadyFlag(isDataReady);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        M5.dis.fillpix(0xff0000);
        return;
    }
    if (!isDataReady) {
        M5.dis.fillpix(0xff0000);

        return;
    }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        M5.dis.fillpix(0xff0000);

    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
        M5.dis.fillpix(0xff0000);
    } else {
        M5.dis.fillpix(0x00ff00);
        Serial.printf("Co2:%d\n", co2);
        Serial.printf("Temperature:%f\n", temperature);
        Serial.printf("Humidity:%f\n\n", humidity);
    }
}
