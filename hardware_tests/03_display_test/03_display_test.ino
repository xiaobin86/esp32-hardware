/**
 * ============================================
 * 03_display_test.ino
 * 四足机器马 - SPI 显示屏测试 (ST7789/ILI9341)
 * ============================================
 * 
 * 功能说明：
 *   - 初始化 SPI 显示屏
 *   - 显示颜色填充、几何图形、文字
 *   - 模拟姿态角数据显示界面
 * 
 * 接线：
 *   显示屏 VCC -> 3.3V
 *   显示屏 GND -> GND
 *   显示屏 SCL/SCK -> GPIO 36 (ESP32-S3 SPI CLK)
 *   显示屏 SDA/MOSI -> GPIO 35 (ESP32-S3 SPI MOSI)
 *   显示屏 RES/RST -> GPIO 38
 *   显示屏 DC        -> GPIO 33
 *   显示屏 CS        -> GPIO 34
 *   显示屏 BLK/LED   -> GPIO 37 (背光，可接 PWM)
 * 
 * 需要的库：
 *   - TFT_eSPI by Bodmer (通过 Arduino IDE 库管理器安装)
 * 
 * 重要配置：
 *   安装 TFT_eSPI 库后，必须修改 User_Setup.h 或创建 User_Setup_Select.h
 *   配置你的显示屏驱动和引脚！
 */

#include <TFT_eSPI.h>

// ============================================
// 创建 TFT 对象
// ============================================
TFT_eSPI tft = TFT_eSPI();

// ============================================
// 颜色定义
// ============================================
#define COLOR_BG    TFT_BLACK
#define COLOR_TEXT  TFT_WHITE
#define COLOR_ACCEL TFT_GREEN
#define COLOR_GYRO  TFT_YELLOW
#define COLOR_MAG   TFT_CYAN
#define COLOR_PITCH TFT_ORANGE
#define COLOR_ROLL  TFT_MAGENTA
#define COLOR_YAW   TFT_BLUE
#define COLOR_BORDER TFT_DARKGREY

// ============================================
// 屏幕尺寸（根据你的屏幕修改）
// ============================================
// ST7789 常见尺寸: 240x240, 240x320, 135x240
// ILI9341 常见尺寸: 240x320
const int SCREEN_WIDTH  = 240;
const int SCREEN_HEIGHT = 240;

// ============================================
// 模拟数据（实际项目中从传感器读取）
// ============================================
float pitch = 0, roll = 0, yaw = 0;
float ax = 0, ay = 0, az = 1.0;
float gx = 0, gy = 0, gz = 0;

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 100; // 100ms 更新一次

// ============================================
// 初始化
// ============================================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n============================================");
  Serial.println("   四足机器马 - 显示屏测试");
  Serial.println("============================================");
  
  // 初始化显示屏
  Serial.println("[初始化] 正在启动 TFT 显示屏...");
  tft.init();
  
  // 设置屏幕方向
  // 0=0°, 1=90°, 2=180°, 3=270°
  tft.setRotation(0);
  
  // 清屏
  tft.fillScreen(COLOR_BG);
  
  Serial.println("[成功] 显示屏初始化完成！");
  Serial.printf("         分辨率: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
  
  // 显示开机画面
  showBootScreen();
  delay(2000);
  
  // 清屏并绘制主界面
  tft.fillScreen(COLOR_BG);
  drawMainLayout();
  
  Serial.println("[就绪] 显示测试开始\n");
}

// ============================================
// 主循环
// ============================================
void loop() {
  unsigned long now = millis();
  
  if (now - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = now;
    
    // 生成模拟数据（实际使用时应从传感器读取）
    generateSimulatedData();
    
    // 更新显示
    updateSensorDisplay();
    updateAttitudeDisplay();
    updateGraphDisplay();
  }
}

// ============================================
// 开机画面
// ============================================
void showBootScreen() {
  // 标题背景
  tft.fillRect(0, 0, SCREEN_WIDTH, 60, TFT_DARKGREEN);
  
  // 标题文字
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);  // 中中对齐
  tft.drawString("四足机器马", SCREEN_WIDTH/2, 25);
  
  tft.setTextSize(1);
  tft.drawString("ESP32-S3 控制系统", SCREEN_WIDTH/2, 50);
  
  // 版本信息
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextDatum(TC_DATUM);  // 顶中对齐
  tft.drawString("硬件测试程序 v1.0", SCREEN_WIDTH/2, 80);
  
  // 硬件清单
  tft.setTextDatum(TL_DATUM);  // 顶左对齐
  tft.setCursor(10, 110);
  tft.println("硬件清单:");
  tft.println("- ESP32-S3 DevKitC");
  tft.println("- MPU9250 姿态传感器");
  tft.println("- OV2640 摄像头");
  tft.println("- 4x MG996R 舵机");
  
  // 状态指示
  tft.setTextColor(TFT_GREEN, COLOR_BG);
  tft.setCursor(10, 200);
  tft.print("状态: ");
  tft.setTextColor(TFT_YELLOW, COLOR_BG);
  tft.print("系统启动中...");
  
  // 进度条动画
  for (int i = 0; i <= 100; i += 10) {
    drawProgressBar(20, 220, 200, 12, i);
    delay(100);
  }
}

// ============================================
// 绘制主界面布局
// ============================================
void drawMainLayout() {
  // 标题栏
  tft.fillRect(0, 0, SCREEN_WIDTH, 30, TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("姿态监控", SCREEN_WIDTH/2, 15);
  
  // 传感器数据区域边框
  tft.drawRect(5, 35, SCREEN_WIDTH-10, 80, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.setCursor(8, 38);
  tft.print("传感器数据");
  
  // 姿态角区域边框
  tft.drawRect(5, 120, SCREEN_WIDTH-10, 60, COLOR_BORDER);
  tft.setCursor(8, 123);
  tft.print("姿态角");
  
  // 图形区域边框
  tft.drawRect(5, 185, SCREEN_WIDTH-10, 50, COLOR_BORDER);
  tft.setCursor(8, 188);
  tft.print("姿态可视化");
}

// ============================================
// 更新传感器数据显示
// ============================================
void updateSensorDisplay() {
  int x = 10;
  int y = 52;
  int lineHeight = 18;
  
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  
  // 加速度计
  tft.setTextColor(COLOR_ACCEL, COLOR_BG);
  tft.setCursor(x, y);
  tft.printf("ACC: %.2f  %.2f  %.2f", ax, ay, az);
  
  // 陀螺仪
  tft.setTextColor(COLOR_GYRO, COLOR_BG);
  tft.setCursor(x, y + lineHeight);
  tft.printf("GYR: %.1f  %.1f  %.1f", gx, gy, gz);
  
  // 磁力计
  tft.setTextColor(COLOR_MAG, COLOR_BG);
  tft.setCursor(x, y + lineHeight * 2);
  tft.printf("MAG: %.1f  %.1f  %.1f", ax*100, ay*100, az*100);
}

// ============================================
// 更新姿态角显示
// ============================================
void updateAttitudeDisplay() {
  int x = 10;
  int y = 137;
  int lineHeight = 18;
  
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  
  // Pitch
  tft.setTextColor(COLOR_PITCH, COLOR_BG);
  tft.setCursor(x, y);
  tft.printf("Pitch: %6.1f", pitch);
  
  // Roll
  tft.setTextColor(COLOR_ROLL, COLOR_BG);
  tft.setCursor(x + 80, y);
  tft.printf("Roll: %6.1f", roll);
  
  // Yaw
  tft.setTextColor(COLOR_YAW, COLOR_BG);
  tft.setCursor(x + 160, y);
  tft.printf("Yaw: %6.1f", yaw);
}

// ============================================
// 更新姿态图形显示
// ============================================
void updateGraphDisplay() {
  int centerX = SCREEN_WIDTH / 2;
  int centerY = 210;
  int radius = 18;
  
  // 清除旧图形区域（只清除内部，保留边框）
  tft.fillRect(6, 199, SCREEN_WIDTH-12, 35, COLOR_BG);
  
  // 绘制水平参考线
  tft.drawFastHLine(centerX - 20, centerY, 40, COLOR_BORDER);
  
  // 根据 Pitch 和 Roll 绘制倾斜指示器
  float radPitch = pitch * PI / 180.0;
  float radRoll = roll * PI / 180.0;
  
  // 计算圆点偏移（限制在范围内）
  int offsetX = (int)(sin(radRoll) * radius);
  int offsetY = (int)(sin(radPitch) * radius);
  
  // 限制范围
  offsetX = constrain(offsetX, -radius, radius);
  offsetY = constrain(offsetY, -radius, radius);
  
  // 绘制圆点
  tft.fillCircle(centerX + offsetX, centerY + offsetY, 5, COLOR_PITCH);
  tft.drawCircle(centerX + offsetX, centerY + offsetY, 5, COLOR_TEXT);
  
  // 绘制倾斜指示线
  int lineLen = 15;
  float angle = atan2(offsetY, offsetX);
  int x2 = centerX + offsetX + (int)(cos(angle) * lineLen);
  int y2 = centerY + offsetY + (int)(sin(angle) * lineLen);
  tft.drawLine(centerX + offsetX, centerY + offsetY, x2, y2, COLOR_ROLL);
}

// ============================================
// 绘制进度条
// ============================================
void drawProgressBar(int x, int y, int width, int height, int percent) {
  // 背景
  tft.drawRect(x, y, width, height, COLOR_BORDER);
  
  // 填充
  int fillWidth = (width - 2) * percent / 100;
  tft.fillRect(x + 1, y + 1, fillWidth, height - 2, TFT_GREEN);
  tft.fillRect(x + 1 + fillWidth, y + 1, width - 2 - fillWidth, height - 2, COLOR_BG);
  
  // 百分比文字
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(String(percent) + "%", x + width/2, y + height/2);
}

// ============================================
// 生成模拟数据（用于测试显示效果）
// ============================================
void generateSimulatedData() {
  static float t = 0;
  t += 0.05;
  
  // 模拟姿态角波动
  pitch = 5.0 * sin(t);
  roll = 3.0 * cos(t * 0.7);
  yaw = fmod(yaw + 0.5, 360.0);
  
  // 模拟传感器数据
  ax = 0.1 * sin(t * 2);
  ay = 0.05 * cos(t * 1.5);
  az = 1.0 + 0.1 * sin(t * 0.5);
  
  gx = 10.0 * cos(t * 2);
  gy = 8.0 * sin(t * 1.5);
  gz = 2.0 * sin(t * 0.3);
}
