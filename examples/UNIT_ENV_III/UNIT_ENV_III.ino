#include "Wire.h"
#include "UNIT_ENV.h"

SHT3X sht30;
QMP6988 qmp6988;

char draw_string[1024];

void setup() {
  Wire.begin(21, 22);
  qmp6988.init();
}

void loop() {
  if (sht30.get() != 0) {
    return;
  }
  Serial.printf("Temperatura: %2.2f*C  Humedad: %0.2f%%  Pressure: %0.2fPa\r\n", sht30.cTemp, sht30.humidity, qmp6988.calcPressure());
  delay(1000);
}
