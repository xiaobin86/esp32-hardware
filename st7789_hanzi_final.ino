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

// GB2312 编码
uint8_t hz_zhong[] = {0xD6, 0xD0};  // "中" - Zone 2
uint8_t hz_guo[]   = {0xB9, 0xFA};  // "国" - Zone 2
uint8_t hz_ni[]    = {0xC4, 0xE3};  // "你" - Zone 2
uint8_t hz_hao[]   = {0xBA, 0xC3};  // "好" - Zone 2

// ========== 修正后的地址计算（分Zone）==========
uint32_t getHanziAddr(uint8_t high, uint8_t low) {
  uint8_t MSB = high;
  uint8_t LSB = low;
  uint32_t BaseAdd = 0x2C9D0;
  uint32_t address;
  
  Serial.printf("\n[ADDR] GB2312: 0x%02X%02X\n", MSB, LSB);
  
  // Zone 1: 符号区 (A1A1 ~ A9EF)
  if ((MSB >= 0xA1) && (MSB <= 0xA9) && (LSB >= 0xA1)) {
    address = ((MSB - 0xA1) * 94 + (LSB - 0xA1)) * 32 + BaseAdd;
    Serial.printf("[ADDR] Zone 1 (符号): (%d-0xA1)*94 + (%d-0xA1) = %d\n", MSB, LSB, 
                  ((MSB - 0xA1) * 94 + (LSB - 0xA1)));
  }
  // Zone 2: 汉字区 (B0A1 ~ F7FE) - "中""国"等常用汉字在此区
  else if ((MSB >= 0xB0) && (MSB <= 0xF7) && (LSB >= 0xA1)) {
    address = ((MSB - 0xB0) * 94 + (LSB - 0xA1) + 846) * 32 + BaseAdd;
    Serial.printf("[ADDR] Zone 2 (汉字): (%d-0xB0)*94 + (%d-0xA1) + 846 = %d\n", MSB, LSB,
                  ((MSB - 0xB0) * 94 + (LSB - 0xA1) + 846));
  }
  else {
    Serial.println("[ADDR] 错误：不在GB2312范围内");
    return 0;
  }
  
  Serial.printf("[ADDR] 最终地址: 0x%06X\n", address);
  return address;
}

// ========== 读取字库（修复SPI冲突）==========
void readFont(uint8_t* hz) {
  uint32_t addr = getHanziAddr(hz[0], hz[1]);
  if (addr == 0) return;
  
  // 重新初始化SPI（修复tft.init()后的冲突）
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  delayMicroseconds(10);
  
  digitalWrite(FONT_CS, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  SPI.transfer(0x03);                      // 读命令
  SPI.transfer((addr >> 16) & 0xFF);       // 地址高字节
  SPI.transfer((addr >>  8) & 0xFF);       // 地址中字节
  SPI.transfer( addr        & 0xFF);       // 地址低字节
  
  // 读取32字节(16×16点阵)
  Serial.print("[DATA] 读取: ");
  int nonZero = 0;
  for (int i = 0; i < 32; i++) {
    fontBuffer[i] = SPI.transfer(0x00);
    Serial.printf("%02X ", fontBuffer[i]);
    if (fontBuffer[i] != 0) nonZero++;
  }
  Serial.println();
  
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
  
  Serial.printf("[INFO] 非零: %d/32 %s\n", nonZero, nonZero > 0 ? "✓" : "✗");
}

// ========== 可视化点阵 ==========
void printBitmap() {
  Serial.println("[FONT] 16×16点阵:");
  int total = 0;
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    Serial.printf("%2d: ", row);
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (left & (0x80 >> bit)) { Serial.print("■"); total++; }
      else Serial.print("□");
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (right & (0x80 >> bit)) { Serial.print("■"); total++; }
      else Serial.print("□");
    }
    Serial.println();
  }
  Serial.printf("[FONT] 亮点: %d/256\n", total);
}

// ========== 绘制汉字 ==========
void drawHanzi(int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t* hz) {
  Serial.printf("\n[DRAW] ========== 绘制到(%d,%d) ==========\n", x, y);
  
  readFont(hz);
  printBitmap();
  
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
  
  Serial.println("[DRAW] 完成!");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("   ST7789 + GT30L32S4W 汉字显示");
  Serial.println("   最终修正版：分Zone地址公式");
  Serial.println("========================================");
  
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
  Serial.println("[INIT] 测试方块已画");
  
  // 显示汉字
  Serial.println("\n\n开始显示汉字...\n");
  
  drawHanzi(50, 50, ST77XX_WHITE, ST77XX_BLACK, hz_zhong);   // 中
  drawHanzi(70, 50, ST77XX_WHITE, ST77XX_BLACK, hz_guo);     // 国
  drawHanzi(50, 80, ST77XX_GREEN, ST77XX_BLACK, hz_ni);      // 你
  drawHanzi(70, 80, ST77XX_GREEN, ST77XX_BLACK, hz_hao);     // 好
  drawHanzi(50, 110, ST77XX_YELLOW, ST77XX_BLACK, hz_zhong); // 中
  drawHanzi(70, 110, ST77XX_YELLOW, ST77XX_BLACK, hz_guo);   // 国
  
  Serial.println("\n\n========================================");
  Serial.println("完成！检查屏幕是否显示: 中国 你好 中国");
  Serial.println("========================================");
}

void loop() {}
