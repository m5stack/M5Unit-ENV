/*
*******************************************************************************
* Copyright (c) 2021 by M5Stack
*                  Equipped with M5AtomS3Lite sample source code
*                          配套  M5AtomS3Lite 示例源代码
* Visit for more information: https://docs.m5stack.com/en/core/atom_lite
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/core/atom_lite
*
* Describe: ENV_DH12_BMP280.  环境传感器
* Date: 2023/2/12
*******************************************************************************
  Please connect to Port A,Read temperature, humidity and atmospheric pressure
  and display them on the display Serial
  请连接端口A,读取温度、湿度和大气压强并在串口上显示
  Libraries:
    - [Adafruit_BMP280](https://github.com/adafruit/Adafruit_BMP280_Library)
    - [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
*/

#include <M5AtomS3.h>
#include "M5_ENV.h"
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>

DHT12 dht12;
Adafruit_BMP280 bme;

void setup() {
    M5.begin(false, true, false,
             true);  // Init M5AtomS3 Lite.  初始化M5AtomS3 Lite
    M5.dis.drawpix(0xFFFFE0);
    M5.dis.show();
    Wire.begin();  // Initialize pin.  初始化引脚
    USBSerial.println(F("ENV Unit(DHT12 and BMP280) test"));
}

void loop() {
    while (!bme.begin(0x76)) {
        USBSerial.println(
            "Could not find a valid BMP280 sensor, check wiring!");
        M5.dis.drawpix(0xff0000);
        M5.dis.show();
    }
    M5.dis.drawpix(0x00ff00);
    M5.dis.show();
    float tmp = dht12.readTemperature();  // Store the temperature obtained from
                                          // dht12.  将dht12获取到的温度存储
    float hum = dht12.readHumidity();  // Store the humidity obtained from the
                                       // dht12.  将dht12获取到的湿度存储
    float pressure = bme.readPressure();  // Stores the pressure gained by BMP.
                                          // 存储bmp获取到的压强
    USBSerial.printf(
        "Temp: %2.1f  \r\nHumi: %2.0f%%  \r\nPressure:%2.0fPa\r\n------\n", tmp,
        hum, pressure);

    delay(2000);
}
