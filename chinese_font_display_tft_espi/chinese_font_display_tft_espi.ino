// ============================================
// TFT_eSPI + GT30L32S4W 字库中文显示
// ============================================
// 硬件：ESP32-S3 + ST7789 240×240 + GT30L32S4W字库
// 
// 安装：
// 1. Arduino IDE 库管理器搜索安装 "TFT_eSPI"（by Bodmer）
// 2. 配置 User_Setup.h（见下方说明）
// ============================================

#include <TFT_eSPI.h>
#include <SPI.h>

// 字库芯片引脚
#define FONT_CS    9   // CS2 - 字库片选
#define TFT_MOSI  11   // SDA - SPI数据（与屏幕共用）
#define TFT_SCLK  12   // SCL - SPI时钟（与屏幕共用）
#define TFT_MISO  13   // FSO - SPI MISO（字库回读）

// 创建TFT对象
TFT_eSPI tft = TFT_eSPI();

// 字库缓冲区
uint8_t fontBuffer[32];

// 汉字编码（GB2312）
uint8_t hz_zhong[] = {0xD6, 0xD0};  // 中
uint8_t hz_guo[]   = {0xB9, 0xFA};  // 国
uint8_t hz_ni[]    = {0xC4, 0xE3};  // 你
uint8_t hz_hao[]   = {0xBA, 0xC3};  // 好

// ========== GB2312 字库地址计算 ==========
uint32_t getHanziAddr(uint8_t high, uint8_t low) {
  uint8_t MSB = high;
  uint8_t LSB = low;
  uint32_t BaseAdd = 0x2C9D0;
  
  if ((MSB >= 0xA1) && (MSB <= 0xA9) && (LSB >= 0xA1)) {
    // Zone 1: 符号区
    return ((MSB - 0xA1) * 94 + (LSB - 0xA1)) * 32 + BaseAdd;
  }
  else if ((MSB >= 0xB0) && (MSB <= 0xF7) && (LSB >= 0xA1)) {
    // Zone 2: 汉字区（+846偏移！）
    return ((MSB - 0xB0) * 94 + (LSB - 0xA1) + 846) * 32 + BaseAdd;
  }
  return 0;
}

// ========== 读取字库（独立SPI实例）==========
// TFT_eSPI使用自己的SPI配置，我们用独立SPI读取字库
SPIClass fontSPI(HSPI);  // 使用HSPI（TFT_eSPI默认用VSPI/FSPI）

void readFont(uint8_t* hz) {
  uint32_t addr = getHanziAddr(hz[0], hz[1]);
  if (addr == 0) return;
  
  Serial.printf("Reading font at 0x%06X\n", addr);
  
  // 初始化字库SPI
  fontSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, FONT_CS);
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  delayMicroseconds(10);
  
  // 片选低电平
  digitalWrite(FONT_CS, LOW);
  delayMicroseconds(10);
  
  fontSPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  // 发送读指令和地址
  fontSPI.transfer(0x03);                          // Read Data
  fontSPI.transfer((addr >> 16) & 0xFF);           // 地址高字节
  fontSPI.transfer((addr >>  8) & 0xFF);           // 地址中字节
  fontSPI.transfer( addr        & 0xFF);           // 地址低字节
  
  // 读取32字节点阵数据
  for (int i = 0; i < 32; i++) {
    fontBuffer[i] = fontSPI.transfer(0x00);
  }
  
  fontSPI.endTransaction();
  digitalWrite(FONT_CS, HIGH);
  fontSPI.end();  // 关闭字库SPI，避免干扰TFT
  
  // 打印点阵调试用
  Serial.println("Font bitmap:");
  for (int row = 0; row < 16; row++) {
    uint8_t left = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    for (int bit = 7; bit >= 0; bit--) {
      Serial.print((left & (1 << bit)) ? "██" : "  ");
    }
    for (int bit = 7; bit >= 0; bit--) {
      Serial.print((right & (1 << bit)) ? "██" : "  ");
    }
    Serial.println();
  }
}

// ========== 绘制汉字（使用TFT_eSPI pushImage）==========
// pushImage是最高效的绘制方式，直接推送像素数组
void drawHanzi(int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t* hz) {
  readFont(hz);
  
  // 构建16×16像素的位图数组（每行16像素）
  uint16_t pixelMap[16][16];
  
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = fontBuffer[row * 2];
    uint8_t right = fontBuffer[row * 2 + 1];
    
    for (uint8_t bit = 0; bit < 8; bit++) {
      pixelMap[row][bit]    = (left  & (0x80 >> bit)) ? color : bg;
      pixelMap[row][8+bit]  = (right & (0x80 >> bit)) ? color : bg;
    }
  }
  
  // 使用pushImage高效推送（16×16像素）
  // pushImage需要一维数组，每行连续
  uint16_t lineBuffer[16];
  for (uint8_t row = 0; row < 16; row++) {
    memcpy(lineBuffer, pixelMap[row], 32);  // 16像素 × 2字节
    tft.pushImage(x, y + row, 16, 1, lineBuffer);
  }
}

// ========== 备选：使用drawPixel（兼容性最好）==========
void drawHanziPixel(int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t* hz) {
  readFont(hz);
  
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("TFT_eSPI + GT30L32S4W Font Demo");
  
  // 初始化TFT（TFT_eSPI自动配置SPI）
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  // 开启背光
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  
  // 字库片选初始化
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  
  // 画参考标记
  tft.drawRect(48, 48, 20, 20, TFT_RED);
  tft.drawRect(68, 48, 20, 20, TFT_GREEN);
  tft.drawRect(88, 48, 20, 20, TFT_BLUE);
  tft.drawRect(108, 48, 20, 20, TFT_YELLOW);
  
  // 显示"中国你好"
  Serial.println("Drawing: 中");
  drawHanzi(50, 50, TFT_WHITE, TFT_BLACK, hz_zhong);
  
  Serial.println("Drawing: 国");
  drawHanzi(70, 50, TFT_WHITE, TFT_BLACK, hz_guo);
  
  Serial.println("Drawing: 你");
  drawHanzi(90, 50, TFT_WHITE, TFT_BLACK, hz_ni);
  
  Serial.println("Drawing: 好");
  drawHanzi(110, 50, TFT_WHITE, TFT_BLACK, hz_hao);
  
  // 测试不同颜色
  drawHanzi(50, 80, TFT_RED, TFT_BLACK, hz_zhong);
  drawHanzi(70, 80, TFT_GREEN, TFT_BLACK, hz_guo);
  drawHanzi(90, 80, TFT_BLUE, TFT_BLACK, hz_ni);
  drawHanzi(110, 80, TFT_YELLOW, TFT_BLACK, hz_hao);
  
  Serial.println("Done!");
}

void loop() {
  // 可以在这里添加动画或刷新逻辑
}
