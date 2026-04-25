#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL     5
#define FONT_CS    9
#define TFT_MISO  13

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
uint8_t fontBuffer[32];

uint8_t hz_zhong[] = {0xD6, 0xD0};
uint8_t hz_guo[]   = {0xB9, 0xFA};

// ========== 修正后的地址计算（分Zone）==========
uint32_t getHanziAddr(uint8_t high, uint8_t low) {
  uint8_t MSB = high;
  uint8_t LSB = low;
  uint32_t BaseAdd = 0x2C9D0;
  uint32_t address;
  
  if ((MSB >= 0xA1) && (MSB <= 0xA9) && (LSB >= 0xA1)) {
    address = ((MSB - 0xA1) * 94 + (LSB - 0xA1)) * 32 + BaseAdd;
  }
  else if ((MSB >= 0xB0) && (MSB <= 0xF7) && (LSB >= 0xA1)) {
    address = ((MSB - 0xB0) * 94 + (LSB - 0xA1) + 846) * 32 + BaseAdd;
  }
  else {
    return 0;
  }
  
  return address;
}

// ========== 读取字库（关键修复：保存/恢复SPI设置）==========
void readFont(uint8_t* hz) {
  uint32_t addr = getHanziAddr(hz[0], hz[1]);
  if (addr == 0) return;
  
  // 保存当前SPI设置（屏幕可能正在使用）
  SPI.end();
  
  // 重新初始化SPI用于字库
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  delayMicroseconds(10);
  
  digitalWrite(FONT_CS, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >>  8) & 0xFF);
  SPI.transfer( addr        & 0xFF);
  
  for (int i = 0; i < 32; i++) {
    fontBuffer[i] = SPI.transfer(0x00);
  }
  
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
  
  // 关键：恢复屏幕的SPI设置
  SPI.end();
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  
  // 重新选中屏幕（让tft对象知道SPI已恢复）
  digitalWrite(TFT_CS, LOW);
  delayMicroseconds(1);
  digitalWrite(TFT_CS, HIGH);
}

// ========== 绘制汉字（使用setAddrWindow批量写入）==========
void drawHanzi(int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t* hz) {
  readFont(hz);
  
  // 使用setAddrWindow + pushColors（比drawPixel快且稳定）
  tft.setAddrWindow(x, y, x + 15, y + 15);
  
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    
    uint16_t linePixels[16];
    for (uint8_t bit = 0; bit < 8; bit++) {
      linePixels[bit]    = (left  & (0x80 >> bit)) ? color : bg;
      linePixels[8+bit]  = (right & (0x80 >> bit)) ? color : bg;
    }
    
    // 批量写入16个像素
    tft.pushColors(linePixels, 16);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  
  // 测试方块
  tft.fillRect(0, 0, 20, 20, ST77XX_RED);
  tft.fillRect(220, 0, 20, 20, ST77XX_GREEN);
  tft.fillRect(0, 220, 20, 20, ST77XX_BLUE);
  
  // 先画一个测试点确认tft正常
  tft.drawPixel(120, 120, ST77XX_WHITE);
  
  // 显示汉字
  drawHanzi(50, 50, ST77XX_WHITE, ST77XX_BLACK, hz_zhong);
  drawHanzi(70, 50, ST77XX_WHITE, ST77XX_BLACK, hz_guo);
  
  // 再画一个测试点确认字库读取后tft仍正常
  tft.drawPixel(122, 122, ST77XX_YELLOW);
}

void loop() {}
