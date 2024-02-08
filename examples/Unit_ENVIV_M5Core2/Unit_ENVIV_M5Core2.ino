/*
*******************************************************************************
* Copyright (c) 2023 by M5Stack
*                  Equipped with M5Core2 sample source code
*                          配套  M5Core2 示例源代码
* Visit to get information: https://docs.m5stack.com/en/unit/ENV%E2%85%A3%20Unit
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/ENV%E2%85%A3%20Unit
*
* Product: ENVIV_SHT40_BMP280.  环境传感器
* Date: 2023/8/24
*******************************************************************************
  Please connect to Port,Read temperature, humidity and atmospheric pressure and
  display them on the display screen
  请连接端口,读取温度、湿度和大气压强并在显示屏上显示
    Libraries:
  - [Adafruit_BMP280](https://github.com/adafruit/Adafruit_BMP280_Library)
  - [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
  - [SensirionI2CSht4x](https://github.com/Tinyu-Zhao/arduino-i2c-sht4x)
*/
#include <M5Core2.h>
#include <SensirionI2CSht4x.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_Sensor.h"

// 初始化传感器
Adafruit_BMP280 bmp;

SensirionI2CSht4x sht4x;
float temperature, pressure,
    humidity; // Store the vuale of pressure and Temperature.  存储压力和温度

void setup()
{
    // 初始化传感器
    M5.begin();
    M5.Lcd.setTextSize(2);
    Wire.begin(); // SDA = 16, SCL = 34
    Serial.begin(115200);
    while (!Serial)
    {
        delay(100);
    }
    while (!bmp.begin(
        0x76))
    { // Init this sensor,True if the init was successful, otherwise
      // false.   初始化传感器,如果初始化成功返回1
        M5.Lcd.println("Could not find a valid BMP280 sensor, check wiring!");
        Serial.println(F("BMP280 fail"));
    }
    M5.Lcd.clear(); // Clear the screen.  清屏
    Serial.println(F("BMP280 test"));

    uint16_t error;
    char errorMessage[256];

    sht4x.begin(Wire);

    uint32_t serialNumber;
    error = sht4x.serialNumber(serialNumber);
    if (error)
    {
        Serial.print("Error trying to execute serialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
    else
    {
        Serial.print("Serial Number: ");
        Serial.println(serialNumber);
    }

    // 设置传感器的采样率和滤波器
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     // 模式：正常
                    Adafruit_BMP280::SAMPLING_X2,     // 温度采样率：2倍
                    Adafruit_BMP280::SAMPLING_X16,    // 压力采样率：16倍
                    Adafruit_BMP280::FILTER_X16,      // 滤波器：16倍
                    Adafruit_BMP280::STANDBY_MS_500); // 等待时间：500毫秒
}

void loop()
{
    uint16_t error;
    char errorMessage[256];

    delay(1000);

    error = sht4x.measureHighPrecision(temperature, humidity);
    if (error)
    {
        Serial.print("Error trying to execute measureHighPrecision(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
    else
    {
        Serial.print("Temperature:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(humidity);
    }
    pressure = bmp.readPressure();
    M5.Lcd.setCursor(0, 0); // 将光标设置在(0 ,0).  Set the cursor to (0,0)
    M5.Lcd.printf("Pressure:%2.0fPa\nTemperature:%2.0f^C", pressure,
                  temperature);
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print("humidity:");
    M5.Lcd.print(humidity);
    M5.Lcd.print("%");
    delay(100);
}
