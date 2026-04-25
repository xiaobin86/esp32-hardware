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

uint8_t hz_zhong[] = {0xD6, 0xD0};  // "中"
uint8_t hz_guo[]   = {0xB9, 0xFA};  // "国"
uint8_t hz_ni[]    = {0xC4, 0xE3};  // "你"
uint8_t hz_hao[]   = {0xBA, 0xC3};  // "好"

// ========== 计算汉字在字库中的地址 ==========
uint32_t getHanziAddr(uint8_t high, uint8_t low) {
  uint8_t qu  = high - 0xA0;   // 区码
  uint8_t wei = low  - 0xA0;   // 位码
  uint32_t base = 0x2C9D0;
  uint32_t offset = ((uint32_t)(qu - 1) * 94 + (wei - 1)) * 32;
  uint32_t addr = base + offset;
  
  Serial.printf("\n[ADDR] GB2312: 0x%02X%02X → 区码=%d, 位码=%d\n", high, low, qu, wei);
  Serial.printf("[ADDR] 计算: 0x%X + ((%d-1)*94 + (%d-1))*32 = 0x%X + 0x%X = 0x%06X\n", 
                base, qu, wei, base, offset, addr);
  
  return addr;
}

// ========== 从字库芯片读取数据（带完整DEBUG）==========
void readFontDebug(uint8_t* hz) {
  uint32_t addr = getHanziAddr(hz[0], hz[1]);
  
  Serial.println("\n[SPI] ========== 开始读字库 ==========");
  
  // 检查并重新初始化 SPI
  Serial.println("[SPI] 重新初始化 SPI.begin(12, 13, 11)");
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  delayMicroseconds(10);
  
  // 拉低 CS2
  Serial.println("[SPI] CS2 (GPIO9) → LOW");
  digitalWrite(FONT_CS, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  // 发送读命令和地址
  Serial.printf("[SPI] 发送命令: 0x03 (读命令)\n");
  Serial.printf("[SPI] 发送地址: 0x%02X 0x%02X 0x%02X\n", 
                (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF);
  
  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >>  8) & 0xFF);
  SPI.transfer( addr        & 0xFF);
  
  // 读取数据
  Serial.print("[SPI] 读取32字节: ");
  bool hasData = false;
  for (int i = 0; i < 32; i++) {
    fontBuffer[i] = SPI.transfer(0x00);
    Serial.printf("%02X ", fontBuffer[i]);
    if (fontBuffer[i] != 0x00) hasData = true;
  }
  Serial.println();
  
  SPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
  Serial.println("[SPI] CS2 → HIGH，读取结束");
  
  // 判断数据有效性
  if (hasData) {
    Serial.println("[✓] 字库数据有效（有非零字节）");
  } else {
    Serial.println("[✗] 警告：字库数据全零！SPI通信可能失败");
  }
}

// ========== 可视化点阵数据 ==========
void printBitmapDebug() {
  Serial.println("\n[FONT] 16×16 点阵图案:");
  int totalPixels = 0;
  
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    
    // 打印行号
    Serial.printf("[FONT] 行%2d: ", row);
    
    // 左8点
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (left & (0x80 >> bit)) {
        Serial.print("■");
        totalPixels++;
      } else {
        Serial.print("□");
      }
    }
    // 右8点
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (right & (0x80 >> bit)) {
        Serial.print("■");
        totalPixels++;
      } else {
        Serial.print("□");
      }
    }
    Serial.println();
  }
  
  Serial.printf("[FONT] 总亮点数: %d / 256\n", totalPixels);
  
  if (totalPixels == 0) {
    Serial.println("[✗] 错误：点阵全空！");
  } else if (totalPixels == 256) {
    Serial.println("[✗] 错误：点阵全满！");
  } else {
    Serial.println("[✓] 点阵数据看起来正常");
  }
}

// ========== 绘制汉字（带DEBUG）==========
void drawHanziDebug(int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t* hz) {
  Serial.printf("\n[DRAW] ========== 绘制汉字 (x=%d, y=%d) ==========\n", x, y);
  
  // 1. 读取字库
  readFontDebug(hz);
  
  // 2. 打印点阵
  printBitmapDebug();
  
  // 3. 绘制到屏幕（逐点绘制）
  Serial.printf("[DRAW] 开始绘制到屏幕坐标(%d,%d)，颜色=0x%04X，背景=0x%04X\n", x, y, color, bg);
  
  int drawnPixels = 0;
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    
    for (uint8_t bit = 0; bit < 8; bit++) {
      bool pixelOn = left & (0x80 >> bit);
      if (pixelOn) drawnPixels++;
      tft.drawPixel(x + bit, y + row, pixelOn ? color : bg);
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      bool pixelOn = right & (0x80 >> bit);
      if (pixelOn) drawnPixels++;
      tft.drawPixel(x + 8 + bit, y + row, pixelOn ? color : bg);
    }
  }
  
  Serial.printf("[DRAW] 绘制完成！实际画了 %d 个彩色像素\n", drawnPixels);
}

void setup() {
  Serial.begin(115200);
  delay(2000);  // 等待串口就绪
  
  Serial.println("========================================");
  Serial.println("   ST7789 + GT30L32S4W 汉字显示测试");
  Serial.println("   带完整 DEBUG 输出");
  Serial.println("========================================");
  
  // 初始化背光
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  Serial.println("[INIT] 背光已开启");
  
  // 初始化字库CS
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  Serial.println("[INIT] 字库CS初始化完成");
  
  // 初始化屏幕
  Serial.println("[INIT] 正在初始化屏幕 tft.init(240,240)...");
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  Serial.println("[INIT] 屏幕初始化完成，已清屏为黑色");
  
  // 画测试方块确认屏幕正常
  Serial.println("[INIT] 画测试方块...");
  tft.fillRect(0, 0, 20, 20, ST77XX_RED);      // 左上红
  tft.fillRect(220, 0, 20, 20, ST77XX_GREEN);  // 右上绿
  tft.fillRect(0, 220, 20, 20, ST77XX_BLUE);   // 左下蓝
  Serial.println("[INIT] 测试方块已画（检查屏幕是否有红绿蓝方块）");
  delay(1000);
  
  // ========== 显示汉字 ==========
  Serial.println("\n========================================");
  Serial.println("   开始显示汉字");
  Serial.println("========================================");
  
  // 第1组：白色"中国"
  drawHanziDebug(50, 50, ST77XX_WHITE, ST77XX_BLACK, hz_zhong);
  drawHanziDebug(70, 50, ST77XX_WHITE, ST77XX_BLACK, hz_guo);
  
  // 第2组：绿色"你好"
  drawHanziDebug(50, 80, ST77XX_GREEN, ST77XX_BLACK, hz_ni);
  drawHanziDebug(70, 80, ST77XX_GREEN, ST77XX_BLACK, hz_hao);
  
  // 第3组：黄色"中"
  drawHanziDebug(50, 110, ST77XX_YELLOW, ST77XX_BLACK, hz_zhong);
  
  // 第4组：红色"国"
  drawHanziDebug(50, 140, ST77XX_RED, ST77XX_BLACK, hz_guo);
  
  Serial.println("\n========================================");
  Serial.println("   所有汉字绘制完成！");
  Serial.println("========================================");
  Serial.println("请检查屏幕是否显示:");
  Serial.println("- 第1行：白色'中国'");
  Serial.println("- 第2行：绿色'你好'");
  Serial.println("- 第3行：黄色'中'");
  Serial.println("- 第4行：红色'国'");
}

void loop() {
  // 空循环
}
