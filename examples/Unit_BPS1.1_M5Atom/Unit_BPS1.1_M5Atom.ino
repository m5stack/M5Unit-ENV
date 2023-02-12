/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with Atom-Lite/Matrix sample source code
*                          配套  Atom-Lite/Matrix 示例源代码
* Visit for more information: https://docs.m5stack.com/en/unit/BPS(QMP6988)
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/BPS(QMP6988)
*
* Product: Pressure_QMP6988.  气压计
* Date: 2023/2/12
******************************************** ***********************************
  Please connect to Port A,Read atmospheric pressure and temperature and
  display them on the display screen
  Libraries:
  - [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
*/

#include <M5Atom.h>
#include "M5_ENV.h"

QMP6988 qmp6988;

float pressure    = 0.0;
float temperature = 0.0;
void setup() {
    M5.begin();  // Init M5Atom.  初始化M5Atom
    Wire.begin();  // Wire init, adding the I2C bus.  Wire初始化, 加入i2c总线
    qmp6988.init();
}

void loop() {
    pressure    = qmp6988.calcPressure();
    temperature = qmp6988.calcTemperature();
    Serial.printf("Pressure:%0.2fPa   \r\nTemperature: %0.2fC\r\n ", pressure,
                  temperature);
    delay(2000);
}
