#include "M5Stack.h"
#include "Wire.h"
#include "QMP6988.h"
#include "SHT3X.h"

SHT3X sht30;
QMP6988 qmp6988;
TFT_eSprite img = TFT_eSprite(&M5.Lcd);

char draw_string[1024];

void setup() {
  M5.begin(true, false, true, false);
  Wire.begin(21, 22);
  qmp6988.init();
  img.setColorDepth(8);
  img.createSprite(320, 240);
}

void loop() {
  img.fillRect(0, 0, 320, 240, 0x00);

  sprintf(draw_string, "ENV-III");
  img.drawCentreString(draw_string, 160, 10, 4);

  if (sht30.get() != 0) {
    img.pushSprite(0, 0);
    return;
  } 

  sprintf(draw_string, "%.1f", sht30.cTemp);
  img.drawCentreString(draw_string, 160, 60, 6);
  sprintf(draw_string, "%.2f", sht30.humidity);
  img.drawCentreString(draw_string, 160, 120, 6);

  qmp6988.init();
  sprintf(draw_string, "%.3f", qmp6988.calcPressure());
  img.drawCentreString(draw_string, 160, 180, 6);
  img.pushSprite(0, 0);
}
