// ============================================
// ST7789 纯英文显示测试（无字库，无软件SPI）
// ============================================
// 目的：测试是否软件SPI读取字库导致反色问题
// 只使用硬件SPI驱动屏幕，不操作GPIO 11/12/13
// ============================================

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// 屏幕引脚
#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL     5

// 创建TFT对象（硬件SPI）
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// 颜色定义
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_RED     0xF800
#define C_GREEN   0x07E0
#define C_BLUE    0x001F
#define C_YELLOW  0xFFE0
#define C_CYAN    0x07FF
#define C_MAGENTA 0xF81F
#define C_ORANGE  0xFC00
#define C_GRAY    0x8410

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ST7789 English Only Test - No Font SPI");
  
  // 初始化GPIO
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // 初始化TFT（硬件SPI）
  tft.init(240, 240);
  tft.setRotation(0);
  
  // 注意：这里不调用 invertDisplay(true)
  // 看看 init() 是否已经自动处理了IPS反色
  
  // 清屏为黑色
  tft.fillScreen(C_BLACK);
  
  Serial.println("TFT initialized");
  
  // ========== 颜色测试 ==========
  
  // 标题
  tft.setTextColor(C_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Color Test");
  
  // 红绿蓝三原色方块
  tft.fillRect(10, 40, 60, 40, C_RED);
  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);
  tft.setCursor(15, 55);
  tft.print("RED");
  
  tft.fillRect(80, 40, 60, 40, C_GREEN);
  tft.setTextColor(C_BLACK);
  tft.setCursor(85, 55);
  tft.print("GREEN");
  
  tft.fillRect(150, 40, 60, 40, C_BLUE);
  tft.setTextColor(C_WHITE);
  tft.setCursor(155, 55);
  tft.print("BLUE");
  
  // 文字颜色测试
  tft.setTextSize(2);
  
  tft.setTextColor(C_RED);
  tft.setCursor(10, 100);
  tft.print("Red Text");
  
  tft.setTextColor(C_GREEN);
  tft.setCursor(10, 125);
  tft.print("Green Text");
  
  tft.setTextColor(C_BLUE);
  tft.setCursor(10, 150);
  tft.print("Blue Text");
  
  tft.setTextColor(C_YELLOW);
  tft.setCursor(10, 175);
  tft.print("Yellow Text");
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(10, 200);
  tft.print("Cyan Text");
  
  // 底部状态
  tft.setTextColor(C_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 230);
  tft.print("No invertDisplay() called");
  
  Serial.println("Display done! Check colors:");
  Serial.println("- Top blocks should be: RED, GREEN, BLUE");
  Serial.println("- Text should be: Red, Green, Blue, Yellow, Cyan");
  Serial.println("- If colors look wrong, init() may need invertDisplay(true)");
}

void loop() {
  // 空循环
  delay(1000);
}