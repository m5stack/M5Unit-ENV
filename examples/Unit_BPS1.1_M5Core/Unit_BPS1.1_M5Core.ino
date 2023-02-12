/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with M5Core sample source code
*                          配套  M5Core 示例源代码
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

#include <M5Stack.h>
#include "M5_ENV.h"

QMP6988 qmp6988;

float pressure    = 0.0;
float temperature = 0.0;
void setup() {
    M5.begin();             // Init M5Stack.  初始化M5Stack
    M5.Power.begin();       // Init power  初始化电源模块
    M5.Lcd.setTextSize(2);  //设置字体大小为2.  Set the font size to 2
    Wire.begin();  // Wire init, adding the I2C bus.  Wire初始化, 加入i2c总线
    qmp6988.init();
    M5.Lcd.clear();  // Clear the screen.  清屏
}

void loop() {
    pressure    = qmp6988.calcPressure();
    temperature = qmp6988.calcTemperature();
    M5.lcd.fillRect(0, 20, 100, 60,
                    BLACK);  // Fill the screen with black (to clear the
                             // screen).  将屏幕填充黑色(用来清屏)
    M5.lcd.setCursor(0, 20);
    M5.Lcd.printf("Pressure:%0.2fPa   \r\nTemperature: %0.2fC\r\n ", pressure,
                  temperature);
    delay(2000);
}
