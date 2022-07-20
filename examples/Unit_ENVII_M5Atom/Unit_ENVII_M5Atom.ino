/*
*******************************************************************************
* Copyright (c) 2021 by M5Stack
*                  Equipped with Atom-Lite/Matrix sample source code
*                          配套  Atom-Lite/Matrix 示例源代码
* Visit for more information: https://docs.m5stack.com/en/unit/envII
* 获取更多资料请访问：https://docs.m5stack.com/zh_CN/unit/envII
*
* Product: ENVII_SHT30_BMP280.  环境传感器
* Date: 2022/7/20
*******************************************************************************
  Please connect to Port,Read temperature, humidity and atmospheric pressure and
  display them on the display Serial
  请连接端口,读取温度、湿度和大气压强并在显示屏上显示
  Libraries:
    - [Adafruit_BMP280](https://github.com/adafruit/Adafruit_BMP280_Library)
    - [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
*/

#include <M5Atom.h>
#include "M5_ENV.h"
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>

SHT3X sht30;
Adafruit_BMP280 bme;

float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

void setup() {
    M5.begin(true, false, true);  // Init M5Atom.  初始化 M5Atom
    M5.dis.fillpix(0xFFFFE0);
    Wire.begin(26, 32);  // Initialize pin 26,32.  初始化26,32引脚
    Serial.println(F("ENV Unit(SHT30 and BMP280) test...\n"));
}

void loop() {
    while (!bme.begin(0x76)) {  //初始化bme传感器.  Init the sensor of bme
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
        M5.dis.fillpix(0xff0000);
    }
    M5.dis.fillpix(0x00ff00);
    pressure = bme.readPressure();  // Stores the pressure gained by BMP.
                                    // 存储bmp获取到的压强
    sht30.get();           // Obtain the data of shT30.  获取sht30的数据
    tmp = sht30.cTemp;     // Store the temperature obtained from shT30.
                           // 将sht30获取到的温度存储
    hum = sht30.humidity;  // Store the humidity obtained from the SHT30.
                           // 将sht30获取到的湿度存储
    Serial.printf("Temp: %2.1f\nHumi: %2.0f%%\nPressure:%2.0fPa\n\n", tmp, hum,
                  pressure);
    delay(2000);
}
