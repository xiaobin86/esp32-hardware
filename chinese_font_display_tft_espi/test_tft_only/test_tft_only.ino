// ============================================
// TFT_eSPI 最小测试 - 确认屏幕能点亮
// ============================================
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("TFT_eSPI Test Start");
  
  // 手动控制背光
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  Serial.println("Backlight ON");
  
  // 初始化TFT
  tft.init();
  Serial.println("TFT init done");
  
  tft.setRotation(0);
  Serial.println("Rotation set");
  
  // 填充屏幕
  tft.fillScreen(TFT_RED);
  Serial.println("Screen RED");
  delay(1000);
  
  tft.fillScreen(TFT_GREEN);
  Serial.println("Screen GREEN");
  delay(1000);
  
  tft.fillScreen(TFT_BLUE);
  Serial.println("Screen BLUE");
  delay(1000);
  
  tft.fillScreen(TFT_BLACK);
  Serial.println("Screen BLACK");
  
  // 画一个大方块
  tft.fillRect(50, 50, 100, 100, TFT_WHITE);
  Serial.println("White rect drawn");
  
  // 显示文字
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(60, 160);
  tft.println("TEST OK");
  Serial.println("Text done");
}

void loop() {
  // 闪烁背光测试
  static unsigned long lastToggle = 0;
  static bool blState = true;
  
  if (millis() - lastToggle > 2000) {
    lastToggle = millis();
    blState = !blState;
    digitalWrite(5, blState ? HIGH : LOW);
    Serial.println(blState ? "BL ON" : "BL OFF");
  }
}
