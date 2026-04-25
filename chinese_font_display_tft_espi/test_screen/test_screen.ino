// ============================================
// TFT_eSPI 屏幕测试（带 flush 和错误捕获）
// ============================================
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  
  // 等待串口连接（对 USB-CDC 很重要）
  while (!Serial && millis() < 3000);
  
  Serial.println("\n========================================");
  Serial.println("TFT_eSPI Screen Test - Debug Mode");
  Serial.println("========================================");
  Serial.flush();
  
  // 1. 背光
  Serial.println("Step 1: Turn on backlight");
  Serial.flush();
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  Serial.println("  Backlight ON");
  Serial.flush();
  delay(500);
  
  // 2. 初始化 TFT（关键步骤）
  Serial.println("Step 2: Initialize TFT...");
  Serial.flush();
  
  // 这里如果崩溃，说明 User_Setup.h 有问题
  tft.init();
  
  Serial.println("  tft.init() SUCCESS!");
  Serial.flush();
  
  // 3. 设置旋转
  Serial.println("Step 3: Set rotation");
  Serial.flush();
  tft.setRotation(0);
  Serial.println("  Rotation set to 0");
  Serial.flush();
  
  // 4. 屏幕填充测试
  Serial.println("Step 4: Screen fill test");
  Serial.flush();
  
  Serial.println("  Filling RED...");
  Serial.flush();
  tft.fillScreen(TFT_RED);
  delay(800);
  
  Serial.println("  Filling GREEN...");
  Serial.flush();
  tft.fillScreen(TFT_GREEN);
  delay(800);
  
  Serial.println("  Filling BLUE...");
  Serial.flush();
  tft.fillScreen(TFT_BLUE);
  delay(800);
  
  Serial.println("  Filling BLACK...");
  Serial.flush();
  tft.fillScreen(TFT_BLACK);
  delay(200);
  
  // 5. 绘制测试图案
  Serial.println("Step 5: Draw test pattern");
  Serial.flush();
  
  tft.fillRect(0, 0, 40, 40, TFT_RED);
  tft.fillRect(200, 0, 40, 40, TFT_GREEN);
  tft.fillRect(0, 200, 40, 40, TFT_BLUE);
  tft.fillRect(200, 200, 40, 40, TFT_YELLOW);
  
  tft.drawLine(120, 0, 120, 240, TFT_WHITE);
  tft.drawLine(0, 120, 240, 120, TFT_WHITE);
  tft.drawCircle(120, 120, 50, TFT_CYAN);
  tft.fillCircle(120, 120, 20, TFT_MAGENTA);
  
  Serial.println("  Test pattern drawn");
  Serial.flush();
  
  // 6. 文字测试
  Serial.println("Step 6: Text test");
  Serial.flush();
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(70, 100);
  tft.println("TEST OK");
  
  tft.setTextSize(1);
  tft.setCursor(40, 130);
  tft.println("If you see this,");
  tft.setCursor(40, 140);
  tft.println("TFT_eSPI works!");
  
  Serial.println("  Text drawn");
  Serial.flush();
  
  Serial.println("\n=== All tests completed! ===");
  Serial.println("If screen shows test pattern, User_Setup.h is correct.");
  Serial.flush();
}

void loop() {
  // 每 3 秒闪烁背光
  static unsigned long lastToggle = 0;
  static bool blState = true;
  
  if (millis() - lastToggle > 3000) {
    lastToggle = millis();
    blState = !blState;
    digitalWrite(5, blState ? HIGH : LOW);
    Serial.println(blState ? "Backlight ON" : "Backlight OFF");
    Serial.flush();
  }
}
