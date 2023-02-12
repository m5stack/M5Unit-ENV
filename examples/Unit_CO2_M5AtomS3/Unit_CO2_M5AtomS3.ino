/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with M5AtomS3 sample source code
*                          配套  M5AtomS3 示例源代码
* Visit for more information: https://docs.m5stack.com/en/unit/co2
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/co2
*
* Product: CO2.  二氧化碳
* Date: 2023/2/11
******************************************** ***********************************
  Please connect Unit to the port, Read CO2 content, temperature, humidity and
  display them on the display screen
  请Unit连接至端口,读取二氧化碳含量,温度,适度,并显示在屏幕上
  Libraries:
  - [Sensirion I2C SCD4x](https://github.com/Sensirion/arduino-i2c-scd4x)]
  - [Sensirion Core](https://github.com/Sensirion/arduino-core)
*/

#include <M5AtomS3.h>
#include <SensirionI2CScd4x.h>

SensirionI2CScd4x scd4x;

void setup() {
    M5.begin(true, true, true,
             false);  // Init M5AtomS3.  初始化M5AtomS3
    Wire.begin(2, 1);
    uint16_t error;
    char errorMessage[256];
    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        USBSerial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        USBSerial.println(errorMessage);
        M5.Lcd.printf("Error:%s\n", errorMessage);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        USBSerial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        USBSerial.println(errorMessage);
        M5.Lcd.printf("Error:%s\n", errorMessage);
    }

    USBSerial.println("Waiting for first measurement... (5 sec)");
    M5.Lcd.printf("Waiting for first measurement... (5 sec)\n");
}

void loop() {
    uint16_t error;
    char errorMessage[256];

    delay(1000);

    // Read Measurement
    uint16_t co2      = 0;
    float temperature = 0.0f;
    float humidity    = 0.0f;
    bool isDataReady  = false;
    error             = scd4x.getDataReadyFlag(isDataReady);
    if (error) {
        USBSerial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        USBSerial.println(errorMessage);
        M5.Lcd.printf("Error:%s\n", errorMessage);
        return;
    }
    if (!isDataReady) {
        return;
    }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        USBSerial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        USBSerial.println(errorMessage);
        M5.Lcd.printf("Error:%s\n", errorMessage);

    } else if (co2 == 0) {
        USBSerial.println("Invalid sample detected, skipping.");
        M5.Lcd.printf("Invalid sample detected, skipping.\n");
    } else {
        M5.Lcd.clear();
        USBSerial.printf("Co2:%d\n", co2);
        USBSerial.printf("Temperature:%f\n", temperature);
        USBSerial.printf("Humidity:%f\n\n", humidity);
        M5.Lcd.printf(" Co2:%d\n Temperature:%f\n Humidity:%f\n\n", co2,
                      temperature, humidity);
    }
}
