// ============================================
// TFT_eSPI 串口输出测试（与屏幕无关）
// ============================================
// 目的：确认串口监视器能正确接收数据
// ============================================

void setup() {
  Serial.begin(115200);
  
  // 等待串口连接（USB-CDC 模式需要）
  unsigned long start = millis();
  while (!Serial && millis() - start < 5000) {
    delay(10);
  }
  
  // 即使没有连接也继续，LED闪烁作为备用指示
  pinMode(38, OUTPUT);  // ESP32-S3 板载 RGB LED（通常是 GPIO38）
  
  Serial.println("\n===================================");
  Serial.println("Serial Test - If you see this, serial works!");
  Serial.println("===================================");
  Serial.print("Boot time: ");
  Serial.print(millis());
  Serial.println(" ms");
  
  // 用 LED 闪烁次数表示状态
  for (int i = 0; i < 5; i++) {
    digitalWrite(38, HIGH);
    delay(100);
    digitalWrite(38, LOW);
    delay(100);
  }
  
  Serial.println("LED blink test done");
  Serial.println("If you DON'T see this text:");
  Serial.println("1. Close Serial Monitor");
  Serial.println("2. Open Serial Monitor again");
  Serial.println("3. Press ESP32 RESET button");
  Serial.println("===================================");
}

void loop() {
  static unsigned long lastPrint = 0;
  static int counter = 0;
  
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    counter++;
    
    Serial.print("Alive: ");
    Serial.print(counter);
    Serial.println(" sec");
    
    // LED 闪烁
    digitalWrite(38, counter % 2);
  }
}
