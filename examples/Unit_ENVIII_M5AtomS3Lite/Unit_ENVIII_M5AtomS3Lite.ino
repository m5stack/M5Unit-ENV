/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with M5AtomS3Lite sample source code
*                          配套  M5AtomS3Lite 示例源代码
* Visit for more information: https://docs.m5stack.com/en/unit/envIII
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/envIII
*
* Product: ENVIII_SHT30_QMP6988.  环境传感器
* Date: 2023/2/12
*******************************************************************************
  Please connect to Port,Read temperature, humidity and atmospheric pressure and
  display them on the display Serial
  请连接端口,读取温度、湿度和大气压强并在显示屏上显示
*/
#include <M5AtomS3.h>
#include "M5_ENV.h"

SHT3X sht30(0x44, 2);
QMP6988 qmp6988;

float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

void setup() {
    M5.begin(false, true, false,
             true);    // Init M5AtomS3Lite. 初始化M5AtomS3Lite
    Wire.begin(2, 1);  // Initialize pin.  初始化引脚
    qmp6988.init();
    USBSerial.println(F("ENVIII Unit(SHT30 and QMP6988) test"));
}

void loop() {
    pressure = qmp6988.calcPressure();
    if (sht30.get() == 0) {  // Obtain the data of shT30.  获取sht30的数据
        tmp = sht30.cTemp;   // Store the temperature obtained from shT30.
                             // 将sht30获取到的温度存储
        hum = sht30.humidity;  // Store the humidity obtained from the SHT30.
                               // 将sht30获取到的湿度存储
    } else {
        tmp = 0, hum = 0;
    }
    USBSerial.printf(
        "Temp: %2.1f  \r\nHumi: %2.0f%%  \r\nPressure:%2.0fPa\r\n---\n", tmp,
        hum, pressure);
    delay(2000);
}
