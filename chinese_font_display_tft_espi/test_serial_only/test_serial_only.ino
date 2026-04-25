// ============================================
// 纯串口测试 - 不依赖任何库
// ============================================
void setup() {
  Serial.begin(115200);
  
  // 等待串口就绪（最长5秒）
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 5000) { delay(10); }
  
  // 不管是否连接都输出
  Serial.println("\n========== SERIAL TEST ==========");
  Serial.println("If you can read this, serial works!");
  Serial.print("Millis: ");
  Serial.println(millis());
  Serial.println("=================================");
}

void loop() {
  static int counter = 0;
  Serial.print("Tick: ");
  Serial.println(counter++);
  delay(1000);
}
