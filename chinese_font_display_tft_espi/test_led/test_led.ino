// ============================================
// TFT_eSPI 屏幕测试 - 简化版（无串口输出依赖）
// ============================================
// 如果串口一直看不到输出，用这个版本：
// 通过 LED 闪烁模式判断执行到哪一步
// ============================================

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// LED 引脚（ESP32-S3 DevKitC-1 板载 RGB LED）
#define LED_PIN 38

void setup() {
  // 初始化 LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // 背光
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  
  // 快速闪烁 3 次 = 程序开始运行
  blinkLED(3, 200);
  delay(500);
  
  // 初始化 TFT
  tft.init();
  
  // 慢速闪烁 2 次 = tft.init() 成功
  blinkLED(2, 500);
  
  tft.setRotation(0);
  
  // 红色 2 秒
  tft.fillScreen(TFT_RED);
  delay(2000);
  
  // 绿色 2 秒
  tft.fillScreen(TFT_GREEN);
  delay(2000);
  
  // 蓝色 2 秒
  tft.fillScreen(TFT_BLUE);
  delay(2000);
  
  // 黑色 + 白色方块
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(80, 80, 80, 80, TFT_WHITE);
  
  // 快速闪烁 5 次 = 全部完成
  blinkLED(5, 100);
}

void loop() {
  // 每秒闪烁一次 = 程序正常运行
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(950);
}

void blinkLED(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(ms);
    digitalWrite(LED_PIN, LOW);
    delay(ms);
  }
}
