#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL     5

// 构造函数直接传针脚，不需要任何配置文件
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);  // 背光开

  tft.init(240, 240);           // 分辨率 240×240
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  // 彩色方块测试
  tft.fillRect(0,   0,  80, 80, ST77XX_RED);
  tft.fillRect(80,  0,  80, 80, ST77XX_GREEN);
  tft.fillRect(160, 0,  80, 80, ST77XX_BLUE);
  tft.fillRect(0,   80, 80, 80, ST77XX_YELLOW);
  tft.fillRect(80,  80, 80, 80, ST77XX_CYAN);
  tft.fillRect(160, 80, 80, 80, ST77XX_MAGENTA);

  // 文字
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 180);
  tft.println("ESP32-S3 OK!");

  tft.setTextSize(1);
  tft.setCursor(10, 210);
  tft.println("ST7789 running");

  Serial.println("done");
}

void loop() {}