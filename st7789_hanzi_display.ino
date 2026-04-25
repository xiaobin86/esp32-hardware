#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ========== 引脚定义 ==========
#define TFT_CS    10    // CS1 - 显示屏片选
#define TFT_DC     6    // DC - 数据/命令
#define TFT_RST    7    // RES - 复位
#define TFT_MOSI  11    // SDA - SPI数据输出(MOSI)
#define TFT_SCLK  12    // SCL - SPI时钟(SCK)
#define TFT_BL     5    // BLK - 背光控制
#define FONT_CS    9    // CS2 - 字库芯片片选
#define TFT_MISO  13    // FSO - SPI数据输入(MISO，字库回读)

// ========== 全局对象 ==========
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
uint8_t fontBuffer[32];  // 16×16点阵 = 32字节

// ========== 汉字定义（十六进制避免编码问题）==========
// GB2312编码
uint8_t hz_zhong[] = {0xD6, 0xD0};  // "中"
uint8_t hz_guo[]   = {0xB9, 0xFA};  // "国"
uint8_t hz_ni[]    = {0xC4, 0xE3};  // "你"
uint8_t hz_hao[]   = {0xBA, 0xC3};  // "好"

// ========== 计算汉字在字库中的地址 ==========
uint32_t getHanziAddr(uint8_t high, uint8_t low) {
  uint8_t qu  = high - 0xA0;   // 区码
  uint8_t wei = low  - 0xA0;   // 位码
  // 16×16点阵起始地址0x2C9D0，每个字32字节
  return 0x2C9D0 + ((uint32_t)(qu - 1) * 94 + (wei - 1)) * 32;
}

// ========== 从字库芯片读取点阵数据 ==========
void readFont(uint8_t* hz) {
  uint32_t addr = getHanziAddr(hz[0], hz[1]);
  
  // 选中字库芯片（CS2拉低）
  digitalWrite(FONT_CS, LOW);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  SPI.transfer(0x03);                      // 读命令
  SPI.transfer((addr >> 16) & 0xFF);       // 地址高字节
  SPI.transfer((addr >>  8) & 0xFF);       // 地址中字节
  SPI.transfer( addr        & 0xFF);       // 地址低字节
  
  for (uint16_t i = 0; i < 32; i++) {
    fontBuffer[i] = SPI.transfer(0x00);    // 读数据
  }
  
  SPI.endTransaction();
  
  // 释放字库芯片（CS2拉高）
  digitalWrite(FONT_CS, HIGH);
}

// ========== 在屏幕指定位置绘制16×16汉字 ==========
// 使用drawPixel逐点绘制，兼容性最好
void drawHanzi(int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t* hz) {
  // 1. 读取字库
  readFont(hz);
  
  // 2. 逐点绘制16×16点阵
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t leftByte  = fontBuffer[row * 2];      // 左8个点
    uint8_t rightByte = fontBuffer[row * 2 + 1];  // 右8个点
    
    // 绘制左半部分（8个点）
    for (uint8_t bit = 0; bit < 8; bit++) {
      // 从最高位开始检查（MSB在前）
      bool pixelOn = leftByte & (0x80 >> bit);
      tft.drawPixel(x + bit, y + row, pixelOn ? color : bg);
    }
    
    // 绘制右半部分（8个点）
    for (uint8_t bit = 0; bit < 8; bit++) {
      bool pixelOn = rightByte & (0x80 >> bit);
      tft.drawPixel(x + 8 + bit, y + row, pixelOn ? color : bg);
    }
  }
}

// ========== 初始化 ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== ST7789 + GT30L32S4W 汉字显示 ===");
  
  // 1. 配置背光引脚
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);   // 开启背光
  Serial.println("[INIT] 背光开启");
  
  // 2. 配置字库片选引脚
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);  // 默认释放字库
  Serial.println("[INIT] 字库CS初始化");
  
  // 3. 初始化屏幕
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  Serial.println("[INIT] 屏幕初始化完成");
  
  // 4. 显示测试信息
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.println("ST7789 Test");
  tft.println("Font Chip Test");
  delay(1000);
  
  // 5. 清空屏幕准备显示汉字
  tft.fillScreen(ST77XX_BLACK);
  
  // ========== 显示汉字 ==========
  
  // 第1行："中国"
  drawHanzi(20, 30, ST77XX_GREEN, ST77XX_BLACK, hz_zhong);   // "中"
  drawHanzi(40, 30, ST77XX_GREEN, ST77XX_BLACK, hz_guo);     // "国"
  
  // 第2行："你好"
  drawHanzi(20, 60, ST77XX_CYAN, ST77XX_BLACK, hz_ni);       // "你"
  drawHanzi(40, 60, ST77XX_CYAN, ST77XX_BLACK, hz_hao);      // "好"
  
  // 第3行：红色"中"
  drawHanzi(20, 90, ST77XX_RED, ST77XX_BLACK, hz_zhong);
  
  // 第4行：黄色"国"
  drawHanzi(20, 120, ST77XX_YELLOW, ST77XX_BLACK, hz_guo);
  
  // 第5行：白色大字（放大效果，每个字间距32像素）
  drawHanzi(80, 160, ST77XX_WHITE, ST77XX_BLACK, hz_ni);
  drawHanzi(100, 160, ST77XX_WHITE, ST77XX_BLACK, hz_hao);
  
  Serial.println("[DRAW] 汉字绘制完成");
  Serial.println("[INFO] 如果屏幕显示汉字，字库功能正常");
  Serial.println("[INFO] 如果看不到汉字，检查CS2(GPIO9)接线");
}

void loop() {
  // 空循环，保持显示
}
