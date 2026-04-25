// ============================================
// TFT_eSPI 屏幕测试 - 最终版
// ============================================
// 如果屏幕仍然不显示，请按顺序尝试：
// 1. 在 User_Setup.h 中取消注释: #define TFT_SPI_MODE SPI_MODE0
// 2. 在 User_Setup.h 中改用: #define ST7789_2_DRIVER
// 3. 在 User_Setup.h 中改成: #define TFT_INVERSION_OFF
// ============================================

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  Serial.println("\n========================================");
  Serial.println("TFT_eSPI Final Test");
  Serial.println("========================================");
  
  // 背光
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  Serial.println("Backlight ON");
  
  // 关键：初始化 TFT
  Serial.println("Calling tft.init()...");
  tft.init();
  Serial.println("tft.init() completed!");
  
  tft.setRotation(0);
  
  // 测试 1：红色（最显眼）
  Serial.println("Filling RED...");
  tft.fillScreen(TFT_RED);
  delay(2000);
  
  // 测试 2：绿色
  Serial.println("Filling GREEN...");
  tft.fillScreen(TFT_GREEN);
  delay(2000);
  
  // 测试 3：蓝色
  Serial.println("Filling BLUE...");
  tft.fillScreen(TFT_BLUE);
  delay(2000);
  
  // 测试 4：白色方块在黑色背景上
  Serial.println("White square on black...");
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(70, 70, 100, 100, TFT_WHITE);
  tft.drawRect(68, 68, 104, 104, TFT_RED);
  
  // 测试 5：文字
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(60, 180);
  tft.print("TEST OK");
  
  Serial.println("All tests done!");
  Serial.println("If screen is still black, try:");
  Serial.println("1. User_Setup.h: uncomment #define TFT_SPI_MODE SPI_MODE0");
  Serial.println("2. User_Setup.h: use #define ST7789_2_DRIVER");
  Serial.println("3. User_Setup.h: change to #define TFT_INVERSION_OFF");
}

void loop() {
  // 每秒闪烁 LED 确认程序在跑
  static unsigned long last = 0;
  if (millis() - last > 1000) {
    last = millis();
    digitalWrite(5, !digitalRead(5));
  }
}
