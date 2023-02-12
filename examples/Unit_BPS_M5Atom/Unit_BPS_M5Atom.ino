/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with Atom-Lite/Matrix sample source code
*                          配套  Atom-Lite/Matrix 示例源代码
* Visit for more information: https://docs.m5stack.com/en/unit/bps
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/bps
*
* Product: BPS_BMP280.  气压计
* Date: 2023/2/7
******************************************** ***********************************
Please connect to the Port, Read atmospheric pressure and temperature
and display them on the display screen Libraries:
  - [Adafruit_BMP280](https://github.com/adafruit/Adafruit_BMP280_Library)
  - [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
*/

#include <M5Atom.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bme;

void setup() {
    M5.begin();  // Init M5Atom.  初始化M5Atom
    Wire.begin();  // Wire init, adding the I2C bus.  Wire初始化, 加入i2c总线
    while (!bme.begin(
        0x76)) {  // Init this sensor,True if the init was successful, otherwise
                  // false.   初始化传感器,如果初始化成功返回1
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    }
}

float pressure,
    Temp;  // Store the vuale of pressure and Temperature.  存储压力和温度

void loop() {
    pressure = bme.readPressure();
    Temp     = bme.readTemperature();
    Serial.printf("Pressure:%2.0fPa\nTemperature:%2.0f^C", pressure, Temp);
    delay(100);
}
