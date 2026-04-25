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
uint8_t hz_zhong[] = {0xD6, 0xD0};  // "中"
uint8_t hz_guo[]   = {0xB9, 0xFA};  // "国"
uint8_t hz_ni[]    = {0xC4, 0xE3};  // "你"
uint8_t hz_hao[]   = {0xBA, 0xC3};  // "好"

// ========== 修正后的地址计算（基地址0x000000）==========
uint32_t getHanziAddr(uint8_t high, uint8_t low) {
  // GB2312: 高字节和低字节都 >= 0xA0
  uint8_t qu  = high - 0xA0;   // 区码 (1-87)
  uint8_t wei = low  - 0xA0;   // 位码 (1-94)
  
  // 汉字从第16区开始，每区94个汉字，每个汉字32字节(16×16点阵)
  uint32_t addr = ((uint32_t)(qu - 16) * 94 + (wei - 1)) * 32;
  
  Serial.printf("\n[ADDR] GB2312: 0x%02X%02X → qu=%d wei=%d\n", high, low, qu, wei);
  Serial.printf("[ADDR] 公式: (%d-16)*94 + (%d-1) = %d * 32 = 0x%X\n", 
                qu, wei, (qu-16)*94 + (wei-1), addr);
  
  return addr;
}

// ========== 读取字库（修正版）==========
void readFont(uint8_t* hz) {
  uint32_t addr = getHanziAddr(hz[0], hz[1]);
  
  // 重新初始化SPI
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
  Serial.print("[DATA] 读取数据: ");
  int nonZero = 0;
  for (int i = 0; i < 32; i++) {
    fontBuffer[i] = SPI.transfer(0x00);
    Serial.printf("%02X ", fontBuffer[i]);
    if (fontBuffer[i] != 0) nonZero++;
  }
  Serial.println();
  
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
  
  Serial.printf("[INFO] 非零字节: %d/32 %s\n", nonZero, nonZero > 0 ? "✓" : "✗");
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
  Serial.printf("[FONT] 总亮点: %d\n", total);
}

// ========== 绘制汉字 ==========
void drawHanzi(int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t* hz) {
  Serial.printf("\n[DRAW] ========== 绘制汉字到(%d,%d) ==========\n", x, y);
  
  readFont(hz);
  printBitmap();
  
  // 绘制
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
  
  Serial.println("[DRAW] 绘制完成!");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("   ST7789 + GT30L32S4W 汉字显示");
  Serial.println("   修正版：基地址=0x000000");
  Serial.println("========================================");
  
  // 初始化
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
  Serial.println("[INIT] 屏幕测试方块已画");
  
  // 显示汉字
  Serial.println("\n\n开始显示汉字...\n");
  
  drawHanzi(50, 50, ST77XX_WHITE, ST77XX_BLACK, hz_zhong);   // 中
  drawHanzi(70, 50, ST77XX_WHITE, ST77XX_BLACK, hz_guo);     // 国
  drawHanzi(50, 80, ST77XX_GREEN, ST77XX_BLACK, hz_ni);      // 你
  drawHanzi(70, 80, ST77XX_GREEN, ST77XX_BLACK, hz_hao);     // 好
  drawHanzi(50, 110, ST77XX_YELLOW, ST77XX_BLACK, hz_zhong); // 中
  drawHanzi(70, 110, ST77XX_YELLOW, ST77XX_BLACK, hz_guo);   // 国
  
  Serial.println("\n\n========================================");
  Serial.println("全部绘制完成！");
  Serial.println("检查屏幕是否显示: 中国 你好 中国");
  Serial.println("========================================");
}

void loop() {}
