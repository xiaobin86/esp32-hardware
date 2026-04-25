// ============================================
// TFT_eSPI 最小测试（修复版）
// ============================================
// User_Setup.h 修改：
// 1. 移除 USE_HSPI_PORT
// 2. 添加 CGRAM_OFFSET
// 3. TFT_RGB_ORDER = TFT_BGR
// 4. TFT_INVERSION_ON
// 5. SPI_FREQUENCY = 40000000
// ============================================

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  Serial.println("\n=== TFT_eSPI Test ===");
  
  // 背光
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  Serial.println("Backlight ON");
  
  // 初始化
  Serial.println("Initializing...");
  tft.init();
  Serial.println("Init done!");
  
  tft.setRotation(0);
  
  // 颜色测试
  Serial.println("RED");
  tft.fillScreen(TFT_RED);
  delay(1000);
  
  Serial.println("GREEN");
  tft.fillScreen(TFT_GREEN);
  delay(1000);
  
  Serial.println("BLUE");
  tft.fillScreen(TFT_BLUE);
  delay(1000);
  
  Serial.println("Test pattern");
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(50, 50, 50, 50, TFT_WHITE);
  tft.drawCircle(120, 120, 40, TFT_YELLOW);
  
  Serial.println("Done!");
}

void loop() {}
