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

// ========== 读取字库（修复SPI）==========
void readFontRaw(uint32_t addr) {
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
  for (int i = 0; i < 32; i++) fontBuffer[i] = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
}

// ========== 绘制点阵 ==========
void drawFontBuffer(int16_t x, int16_t y, uint16_t color, uint16_t bg) {
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    for (uint8_t bit = 0; bit < 8; bit++) {
      tft.drawPixel(x + bit, y + row, (left & (0x80 >> bit)) ? color : bg);
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      tft.drawPixel(x + 8 + bit, y + row, (right & (0x80 >> bit)) ? color : bg);
    }
  }
}

// ========== 打印点阵到串口 ==========
void printFontBuffer() {
  Serial.println("点阵:");
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    for (uint8_t bit = 0; bit < 8; bit++) Serial.print((left & (0x80 >> bit)) ? "■" : "□");
    for (uint8_t bit = 0; bit < 8; bit++) Serial.print((right & (0x80 >> bit)) ? "■" : "□");
    Serial.println();
  }
}

// ========== 用不同基地址和公式尝试 ==========
void testHanzi(uint8_t high, uint8_t low, const char* name, 
               uint32_t baseAddr, int quOffset, int weiOffset, 
               int16_t screenX, int16_t screenY, uint16_t color) {
  
  uint8_t qu  = high - 0xA0;
  uint8_t wei = low  - 0xA0;
  
  // 计算序号
  int index = (qu - quOffset) * 94 + (wei - weiOffset);
  uint32_t addr = baseAddr + index * 32;
  
  Serial.printf("\n[%s] GB2312=0x%02X%02X qu=%d wei=%d\n", name, high, low, qu, wei);
  Serial.printf("公式: 0x%X + (%d-%d)*94 + (%d-%d) = 0x%X + %d*32\n", 
                baseAddr, qu, quOffset, wei, weiOffset, baseAddr, index);
  Serial.printf("地址: 0x%06X\n", addr);
  
  readFontRaw(addr);
  
  // 检查数据
  int nonZero = 0;
  for (int i = 0; i < 32; i++) if (fontBuffer[i] != 0) nonZero++;
  Serial.printf("非零字节: %d/32\n", nonZero);
  
  if (nonZero > 0) {
    printFontBuffer();
    drawFontBuffer(screenX, screenY, color, ST77XX_BLACK);
    Serial.printf("→ 已绘制到屏幕(%d,%d)\n", screenX, screenY);
  } else {
    Serial.println("→ 数据为空，跳过绘制");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("   字库地址公式排查");
  Serial.println("========================================");
  
  // 初始化屏幕
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  
  // 画角落方块确认屏幕
  tft.fillRect(0, 0, 20, 20, ST77XX_RED);
  tft.fillRect(220, 0, 20, 20, ST77XX_GREEN);
  tft.fillRect(0, 220, 20, 20, ST77XX_BLUE);
  
  // ========== "啊"字测试（GB2312第一个汉字 0xB0A1）==========
  Serial.println("\n\n========================================");
  Serial.println("测试 '啊' (0xB0A1) - GB2312第一个汉字");
  Serial.println("========================================");
  
  // 方案A: 基地址0x2C9D0, qu从1开始
  testHanzi(0xB0, 0xA1, "啊-A", 0x2C9D0, 1, 1, 30, 40, ST77XX_WHITE);
  
  // 方案B: 基地址0x2C9D0, qu从16开始(符号区不算)
  testHanzi(0xB0, 0xA1, "啊-B", 0x2C9D0, 16, 1, 50, 40, ST77XX_CYAN);
  
  // 方案C: 基地址0x000000, qu从1开始
  testHanzi(0xB0, 0xA1, "啊-C", 0x000000, 1, 1, 70, 40, ST77XX_YELLOW);
  
  // 方案D: 基地址0x000000, qu从16开始
  testHanzi(0xB0, 0xA1, "啊-D", 0x000000, 16, 1, 90, 40, ST77XX_MAGENTA);
  
  // ========== "中"字测试（0xD6D0）==========
  Serial.println("\n\n========================================");
  Serial.println("测试 '中' (0xD6D0)");
  Serial.println("========================================");
  
  testHanzi(0xD6, 0xD0, "中-A", 0x2C9D0, 1, 1, 30, 80, ST77XX_WHITE);
  testHanzi(0xD6, 0xD0, "中-B", 0x2C9D0, 16, 1, 50, 80, ST77XX_CYAN);
  testHanzi(0xD6, 0xD0, "中-C", 0x000000, 1, 1, 70, 80, ST77XX_YELLOW);
  testHanzi(0xD6, 0xD0, "中-D", 0x000000, 16, 1, 90, 80, ST77XX_MAGENTA);
  
  // ========== "国"字测试（0xB9FA）==========
  Serial.println("\n\n========================================");
  Serial.println("测试 '国' (0xB9FA)");
  Serial.println("========================================");
  
  testHanzi(0xB9, 0xFA, "国-A", 0x2C9D0, 1, 1, 30, 120, ST77XX_WHITE);
  testHanzi(0xB9, 0xFA, "国-B", 0x2C9D0, 16, 1, 50, 120, ST77XX_CYAN);
  testHanzi(0xB9, 0xFA, "国-C", 0x000000, 1, 1, 70, 120, ST77XX_YELLOW);
  testHanzi(0xB9, 0xFA, "国-D", 0x000000, 16, 1, 90, 120, ST77XX_MAGENTA);
  
  Serial.println("\n\n========================================");
  Serial.println("排查完成！");
  Serial.println("请观察屏幕上的字，哪个位置显示正确？");
  Serial.println("A=白色 B=青色 C=黄色 D=洋红");
  Serial.println("========================================");
}

void loop() {}
