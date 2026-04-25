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

// ========== 字库地址计算 ==========
uint32_t getHanziAddr(uint8_t high, uint8_t low) {
  uint8_t qu  = high - 0xA0;
  uint8_t wei = low  - 0xA0;
  uint32_t addr = 0x2C9D0 + ((uint32_t)(qu - 1) * 94 + (wei - 1)) * 32;
  Serial.printf("[ADDR] 区=%d,位=%d → 地址=0x%06X\n", qu, wei, addr);
  return addr;
}

// ========== 步骤1：纯SPI读字库（不依赖任何库）==========
void testFontChipRaw() {
  Serial.println("\n=== 步骤1：原始SPI字库测试 ===");
  
  // 手动控制SPI
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  
  uint32_t addr = getHanziAddr(0xD6, 0xD0);  // "中"
  
  digitalWrite(FONT_CS, LOW);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >>  8) & 0xFF);
  SPI.transfer( addr        & 0xFF);
  
  Serial.print("[RAW] 字库原始数据: ");
  bool hasData = false;
  for (int i = 0; i < 32; i++) {
    fontBuffer[i] = SPI.transfer(0x00);
    Serial.printf("%02X ", fontBuffer[i]);
    if (fontBuffer[i] != 0x00 && fontBuffer[i] != 0xFF) hasData = true;
    if ((i + 1) % 8 == 0) Serial.print("| ");
  }
  Serial.println();
  
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
  
  if (hasData) {
    Serial.println("[✓] 字库有数据，SPI通信正常");
  } else {
    Serial.println("[✗] 字库数据异常！全0或全FF");
    Serial.println("    可能原因：CS2(GPIO9)接错、字库芯片损坏、SPI线松动");
  }
}

// ========== 步骤2：检查屏幕是否能画点 ==========
void testScreenPixel() {
  Serial.println("\n=== 步骤2：屏幕像素测试 ===");
  
  // 在屏幕四个角画点
  tft.drawPixel(0, 0, ST77XX_RED);       // 左上角
  tft.drawPixel(239, 0, ST77XX_GREEN);   // 右上角
  tft.drawPixel(0, 239, ST77XX_BLUE);    // 左下角
  tft.drawPixel(239, 239, ST77XX_WHITE); // 右下角
  
  // 画一个十字
  for (int i = 0; i < 240; i++) {
    tft.drawPixel(i, 120, ST77XX_YELLOW);   // 横线
    tft.drawPixel(120, i, ST77XX_YELLOW);   // 竖线
  }
  
  Serial.println("[✓] 屏幕应该显示：四角彩色点 + 黄色十字线");
  Serial.println("    如果看不到，检查CS1/DC/RES接线");
  delay(2000);
}

// ========== 步骤3：检查字库和屏幕的SPI是否冲突 ==========
void testSPIConflict() {
  Serial.println("\n=== 步骤3：SPI冲突测试 ===");
  
  // 先操作屏幕
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Before Font");
  Serial.println("[INFO] 屏幕显示 'Before Font'");
  delay(1000);
  
  // 再读字库
  uint32_t addr = getHanziAddr(0xD6, 0xD0);
  
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, LOW);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >>  8) & 0xFF);
  SPI.transfer( addr        & 0xFF);
  for (int i = 0; i < 32; i++) fontBuffer[i] = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
  
  // 再操作屏幕
  tft.setCursor(0, 30);
  tft.println("After Font");
  Serial.println("[INFO] 屏幕应该显示 'After Font'");
  
  // 检查屏幕是否还能正常显示
  Serial.println("[?] 如果 'Before Font' 消失或乱码，说明SPI配置被字库读取破坏");
  delay(2000);
}

// ========== 步骤4：绘制汉字（带详细debug）==========
void drawHanziDebug(int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t* hz) {
  Serial.println("\n=== 步骤4：绘制汉字 ===");
  
  uint32_t addr = getHanziAddr(hz[0], hz[1]);
  
  // 读字库
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, LOW);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >>  8) & 0xFF);
  SPI.transfer( addr        & 0xFF);
  for (int i = 0; i < 32; i++) fontBuffer[i] = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
  
  // 打印点阵
  Serial.println("[FONT] 16×16点阵:");
  int pixelCount = 0;
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    
    String line = "";
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (left & (0x80 >> bit)) {
        line += "■";
        pixelCount++;
      } else {
        line += "□";
      }
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (right & (0x80 >> bit)) {
        line += "■";
        pixelCount++;
      } else {
        line += "□";
      }
    }
    Serial.println(line);
  }
  Serial.printf("[FONT] 总亮像素数: %d\n", pixelCount);
  
  if (pixelCount == 0) {
    Serial.println("[✗] 点阵全空！字库数据无效");
    return;
  }
  if (pixelCount == 256) {
    Serial.println("[✗] 点阵全满！可能读到了错误数据");
    return;
  }
  
  // 绘制到屏幕（使用setAddrWindow批量写入）
  Serial.printf("[DRAW] 在(%d,%d)绘制，颜色=0x%04X\n", x, y, color);
  
  tft.setAddrWindow(x, y, x + 15, y + 15);
  
  uint16_t pixelBuf[16];
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    
    for (uint8_t bit = 0; bit < 8; bit++) {
      pixelBuf[bit]   = (left  & (0x80 >> bit)) ? color : bg;
      pixelBuf[8+bit] = (right & (0x80 >> bit)) ? color : bg;
    }
    
    // 批量写入16个像素
    for (int i = 0; i < 16; i++) {
      tft.pushColor(pixelBuf[i]);
    }
  }
  
  Serial.println("[✓] 绘制完成！检查屏幕是否显示汉字");
}

// ========== 步骤5：检查SPI模式是否正确 ==========
void testSPIMode() {
  Serial.println("\n=== 步骤5：SPI模式验证 ===");
  
  // GT30L32S4W 字库芯片支持 SPI MODE0 和 MODE3
  // 测试不同模式下的读取结果
  
  uint32_t addr = getHanziAddr(0xD6, 0xD0);
  
  for (int mode = 0; mode <= 3; mode++) {
    uint8_t testBuf[4];
    
    digitalWrite(FONT_CS, LOW);
    
    switch(mode) {
      case 0: SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); break;
      case 1: SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE1)); break;
      case 2: SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE2)); break;
      case 3: SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE3)); break;
    }
    
    SPI.transfer(0x03);
    SPI.transfer((addr >> 16) & 0xFF);
    SPI.transfer((addr >>  8) & 0xFF);
    SPI.transfer( addr        & 0xFF);
    for (int i = 0; i < 4; i++) testBuf[i] = SPI.transfer(0x00);
    
    SPI.endTransaction();
    digitalWrite(FONT_CS, HIGH);
    
    Serial.printf("[SPI_MODE%d] 前4字节: %02X %02X %02X %02X", 
                  mode, testBuf[0], testBuf[1], testBuf[2], testBuf[3]);
    
    // 检查是否有有效数据（非全0非全FF）
    bool valid = false;
    for (int i = 0; i < 4; i++) {
      if (testBuf[i] != 0x00 && testBuf[i] != 0xFF) valid = true;
    }
    Serial.println(valid ? " ✓" : " ✗");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("   ST7789 + GT30L32S4W 字库排查程序");
  Serial.println("========================================");
  
  // 初始化屏幕
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  
  Serial.println("[INIT] 屏幕初始化完成");
  Serial.printf("[INIT] GPIO配置: CS=%d, DC=%d, RST=%d, BL=%d\n", TFT_CS, TFT_DC, TFT_RST, TFT_BL);
  Serial.printf("[INIT] SPI配置: MOSI=%d, MISO=%d, SCK=%d, CS2=%d\n", TFT_MOSI, TFT_MISO, TFT_SCLK, FONT_CS);
  
  // 执行排查步骤
  testFontChipRaw();      // 步骤1
  testScreenPixel();      // 步骤2
  testSPIConflict();      // 步骤3
  testSPIMode();          // 步骤5
  
  // 最后尝试绘制
  uint8_t zhong[] = {0xD6, 0xD0};
  drawHanziDebug(100, 100, ST77XX_GREEN, ST77XX_BLACK, zhong);
  
  Serial.println("\n=== 排查完成 ===");
  Serial.println("请检查:");
  Serial.println("1. 串口输出的字库数据是否正常");
  Serial.println("2. 屏幕是否显示四角彩色点和黄色十字");
  Serial.println("3. 最终是否显示汉字");
}

void loop() {
  // 空循环
}
