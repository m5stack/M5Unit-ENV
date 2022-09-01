/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with M5Stack sample source code
*                          配套  M5Stack 示例源代码
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

#include <M5Stack.h>
#include <SensirionI2CScd4x.h>

SensirionI2CScd4x scd4x;

void setup() {
    M5.begin();
    M5.Power.begin();
    M5.Lcd.setTextFont(4);
    M5.Lcd.drawString("Unit CO2", 110, 0);
    uint16_t error;
    char errorMessage[256];

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
        M5.Lcd.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        return;
    }
    if (!isDataReady) {
        return;
    }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        M5.Lcd.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (co2 == 0) {
        M5.Lcd.println("Invalid sample detected, skipping.");
    } else {
        M5.Lcd.setCursor(0, 40);
        M5.Lcd.printf("Co2:%d\n", co2);
        M5.Lcd.printf("Temperature:%f\n", temperature);
        M5.Lcd.printf("Humidity:%f\n", humidity);
    }
}
