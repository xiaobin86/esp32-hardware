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

// 12×12点阵缓冲区：12行×2字节 = 24字节
uint8_t fontBuffer12[24];

// ========== 读取字库（按24字节 = 12×12点阵）==========
void readFont12x12(uint32_t addr) {
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
  for (int i = 0; i < 24; i++) {
    fontBuffer12[i] = SPI.transfer(0x00);
  }
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
}

// ========== 打印12×12点阵==========
void print12x12(const char* label) {
  Serial.printf("\n[%s] 12×12点阵 (24字节):\n", label);
  for (int i = 0; i < 24; i++) {
    Serial.printf("%02X ", fontBuffer12[i]);
    if ((i + 1) % 8 == 0) Serial.print("| ");
  }
  Serial.println();
  
  Serial.println("图案:");
  for (uint8_t row = 0; row < 12; row++) {
    uint8_t left  = fontBuffer12[row * 2];
    uint8_t right = fontBuffer12[row * 2 + 1];
    
    // 12×12: 从2字节(16位)中取12位
    // 尝试高12位有效 (bit15~bit4)
    for (int bit = 7; bit >= 4; bit--) {
      Serial.print((left & (1 << bit)) ? "■" : "□");
    }
    for (int bit = 7; bit >= 0; bit--) {
      Serial.print((right & (1 << bit)) ? "■" : "□");
    }
    // 总共: 4+8=12个点 ✓
    Serial.println();
  }
}

// ========== 绘制12×12点阵到屏幕==========
void draw12x12(int16_t x, int16_t y, uint16_t color, uint16_t bg) {
  for (uint8_t row = 0; row < 12; row++) {
    uint8_t left  = fontBuffer12[row * 2];
    uint8_t right = fontBuffer12[row * 2 + 1];
    
    // 高12位有效: 左字节取bit7~bit4 (4个点)，右字节取bit7~bit0 (8个点)
    for (int bit = 7; bit >= 4; bit--) {
      tft.drawPixel(x + (7-bit), y + row, (left & (1 << bit)) ? color : bg);
    }
    for (int bit = 7; bit >= 0; bit--) {
      tft.drawPixel(x + 4 + (7-bit), y + row, (right & (1 << bit)) ? color : bg);
    }
  }
}

// ========== 测试12×12地址公式==========
void test12x12(uint8_t high, uint8_t low, const char* name,
               uint32_t baseAddr, int quOffset, int weiOffset,
               int16_t screenX, int16_t screenY, uint16_t color) {
  
  uint8_t qu  = high - 0xA0;
  uint8_t wei = low  - 0xA0;
  
  // 12×12 = 24字节/字
  int index = (qu - quOffset) * 94 + (wei - weiOffset);
  uint32_t addr = baseAddr + index * 24;
  
  Serial.printf("\n[%s] GB2312=0x%02X%02X qu=%d wei=%d\n", name, high, low, qu, wei);
  Serial.printf("12×12公式: base=0x%X, index=(%d-%d)*94+(%d-%d)=%d\n", 
                baseAddr, qu, quOffset, wei, weiOffset, index);
  Serial.printf("地址: 0x%06X (+%d*24)\n", addr, index);
  
  readFont12x12(addr);
  
  // 检查数据
  int nonZero = 0;
  for (int i = 0; i < 24; i++) if (fontBuffer12[i] != 0) nonZero++;
  Serial.printf("非零字节: %d/24\n", nonZero);
  
  if (nonZero > 0) {
    print12x12(name);
    draw12x12(screenX, screenY, color, ST77XX_BLACK);
    Serial.printf("→ 已绘制12×12到(%d,%d)\n", screenX, screenY);
  } else {
    Serial.println("→ 全零");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("   12×12 字库地址公式排查");
  Serial.println("========================================");
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  
  // 角落方块
  tft.fillRect(0, 0, 20, 20, ST77XX_RED);
  tft.fillRect(220, 0, 20, 20, ST77XX_GREEN);
  tft.fillRect(0, 220, 20, 20, ST77XX_BLUE);
  
  // ========== 测试 "国" (0xB9FA) ==========
  Serial.println("\n\n===== 测试 '国' (0xB9FA) =====");
  
  // 方案A: base=0x2C9D0, qu从1算
  test12x12(0xB9, 0xFA, "国-12-A", 0x2C9D0, 1, 1, 30, 40, ST77XX_WHITE);
  
  // 方案B: base=0x2C9D0, qu从16算
  test12x12(0xB9, 0xFA, "国-12-B", 0x2C9D0, 16, 1, 50, 40, ST77XX_CYAN);
  
  // 方案C: base=0x000000, qu从1算
  test12x12(0xB9, 0xFA, "国-12-C", 0x000000, 1, 1, 70, 40, ST77XX_YELLOW);
  
  // 方案D: base=0x000000, qu从16算
  test12x12(0xB9, 0xFA, "国-12-D", 0x000000, 16, 1, 90, 40, ST77XX_MAGENTA);
  
  // ========== 测试 "中" (0xD6D0) ==========
  Serial.println("\n\n===== 测试 '中' (0xD6D0) =====");
  
  test12x12(0xD6, 0xD0, "中-12-A", 0x2C9D0, 1, 1, 30, 70, ST77XX_WHITE);
  test12x12(0xD6, 0xD0, "中-12-B", 0x2C9D0, 16, 1, 50, 70, ST77XX_CYAN);
  test12x12(0xD6, 0xD0, "中-12-C", 0x000000, 1, 1, 70, 70, ST77XX_YELLOW);
  test12x12(0xD6, 0xD0, "中-12-D", 0x000000, 16, 1, 90, 70, ST77XX_MAGENTA);
  
  // ========== 额外测试：按16×16读但只取前24字节？==========
  // 如果上面都不对，可能是数据排列方式不同
  
  Serial.println("\n\n========================================");
  Serial.println("12×12排查完成！");
  Serial.println("请观察屏幕，哪一列显示了正确的字？");
  Serial.println("A=白 B=青 C=黄 D=洋红");
  Serial.println("========================================");
}

void loop() {}
