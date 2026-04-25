// ============================================
// ST7789 汉字显示 - 修复绘制版本
// ============================================
// 改用批量绘制：setAddrWindow + pushColor
// 避免 drawPixel 的 SPI 事务开销
// ============================================
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
#define FONT_MISO 13
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_RED     0xF800
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
uint8_t fontBuf[32];  // 单个汉字缓冲区
// ========== 软件SPI ==========
void softSPI_WriteByte(uint8_t dat) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(TFT_SCLK, LOW);
    digitalWrite(TFT_MOSI, (dat & 0x80) ? HIGH : LOW);
    digitalWrite(TFT_SCLK, HIGH);
    dat <<= 1;
  }
}
uint8_t softSPI_ReadByte() {
  uint8_t ret = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(TFT_SCLK, LOW);
    ret <<= 1;
    if (digitalRead(FONT_MISO)) ret |= 0x01;
    digitalWrite(TFT_SCLK, HIGH);
  }
  return ret;
}
void readFont(uint32_t addr, uint8_t* buf) {
  digitalWrite(TFT_CS, HIGH);
  delayMicroseconds(50);
  digitalWrite(FONT_CS, LOW);
  delayMicroseconds(50);
  
  softSPI_WriteByte(0x03);
  softSPI_WriteByte((addr >> 16) & 0xFF);
  softSPI_WriteByte((addr >>  8) & 0xFF);
  softSPI_WriteByte( addr        & 0xFF);
  
  for (int i = 0; i < 32; i++) buf[i] = softSPI_ReadByte();
  
  digitalWrite(FONT_CS, HIGH);
  delayMicroseconds(50);
}
// ========== 批量绘制汉字（修复版）==========
// 使用 setAddrWindow + pushColor 一次性发送 256 像素
void drawHanziFast(int16_t x, int16_t y, uint16_t fg, uint16_t bg, uint8_t* buf) {
  // 设置 16x16 的绘制窗口
  tft.startWrite();
  tft.setAddrWindow(x, y, 16, 16);
  
  // 一次性发送所有像素颜色
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = buf[row * 2];
    uint8_t right = buf[row * 2 + 1];
    
    for (uint8_t bit = 0; bit < 8; bit++) {
      tft.pushColor((left & (0x80 >> bit)) ? fg : bg);
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      tft.pushColor((right & (0x80 >> bit)) ? fg : bg);
    }
  }
  
  tft.endWrite();
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // ---------- 阶段1：读取字库 ----------
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
  pinMode(FONT_CS, OUTPUT); digitalWrite(FONT_CS, HIGH);
  pinMode(FONT_MISO, INPUT);
  pinMode(TFT_SCLK, OUTPUT);
  pinMode(TFT_MOSI, OUTPUT);
  digitalWrite(TFT_SCLK, LOW);
  digitalWrite(TFT_MOSI, LOW);
  
  // 读取"你" (0xC4E3)
  uint32_t addr = ((0xC4 - 0xB0) * 94 + (0xE3 - 0xA1) + 846) * 32 + 0x2C9D0;
  readFont(addr, fontBuf);
  
  Serial.println("Font read done");
  
  // ---------- 阶段2：初始化屏幕 ----------
  pinMode(TFT_SCLK, INPUT);
  pinMode(TFT_MOSI, INPUT);
  
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(C_BLACK);
  
  Serial.println("Screen init done");
  
  // ---------- 阶段3：批量绘制 ----------
  // 方法1：批量绘制（推荐）
  drawHanziFast(50, 50, C_WHITE, C_BLACK, fontBuf);
  Serial.println("Hanzi drawn with batch method");
  
  // 标注
  tft.setTextColor(C_GREEN);
  tft.setCursor(10, 100);
  tft.print("Batch draw test");
}
void loop() {
  delay(1000);
}