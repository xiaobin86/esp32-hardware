/**
 * ESP32-S3 + ST7789 + GY-91 (MPU9250 + BMP280) 示例
 * 
 * 功能：读取加速度、陀螺仪、磁力计和气压数据，显示在 ST7789 屏幕上
 * 编码：GB2312（确保中文正常显示）
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <Adafruit_BMP280.h>

// ── ST7789 引脚定义 ──────────────────────────
#define TFT_CS    10   // 屏幕片选
#define TFT_DC     6   // 数据/命令
#define TFT_RST    7   // 复位
#define TFT_MOSI  11   // SPI MOSI
#define TFT_SCLK  12   // SPI CLK
#define TFT_BL     5   // 背光
#define TFT_MISO  13   // SPI MISO（字库用）
#define FONT_CS    9   // 字库片选

// ── GY-91 I2C 引脚定义 ───────────────────────
#define GY91_SDA   3   // I2C 数据
#define GY91_SCL   4   // I2C 时钟

// ── 颜色定义（16位 RGB565）───────────────────
#define COLOR_BG     0x0000  // 黑色背景
#define COLOR_TITLE  0xFFE0  // 黄色标题
#define COLOR_ACCEL  0x07FF  // 青色 - 加速度
#define COLOR_GYRO   0xF81F  // 紫色 - 陀螺仪
#define COLOR_MAG    0x07E0  // 绿色 - 磁力计
#define COLOR_TEMP   0xFFA0  // 橙色 - 温度
#define COLOR_PRESS  0x001F  // 蓝色 - 气压
#define COLOR_VALUE  0xFFFF  // 白色数值

// ── 全局对象 ─────────────────────────────────
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
MPU9250_asukiaaa mpu;
Adafruit_BMP280 bmp;

// ── 数据存储 ─────────────────────────────────
float accelX, accelY, accelZ;
float gyroX, gyroY, gyroZ;
float magX, magY, magZ;
float temperature;
float pressure;
float altitude;

// ── 字库读取函数（GT30L32S4W）────────────────
void fontChipRead(uint32_t addr, uint8_t* buf, uint16_t len) {
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(FONT_CS, LOW);
  
  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >>  8) & 0xFF);
  SPI.transfer( addr        & 0xFF);
  
  for (uint16_t i = 0; i < len; i++) {
    buf[i] = SPI.transfer(0x00);
  }
  
  digitalWrite(FONT_CS, HIGH);
  SPI.endTransaction();
}

// GB2312 汉字地址计算（16×16 点阵）
uint32_t getHanziAddr(uint8_t h, uint8_t l) {
  uint8_t qu  = h - 0xA0;
  uint8_t wei = l - 0xA0;
  return 0x2C9D0 + ((uint32_t)(qu - 1) * 94 + (wei - 1)) * 32;
}

// 绘制单个 16×16 汉字
void drawHanzi(int16_t x, int16_t y, const uint8_t* gb, uint16_t fg, uint16_t bg) {
  uint8_t buf[32];
  fontChipRead(getHanziAddr(gb[0], gb[1]), buf, 32);
  
  for (uint8_t row = 0; row < 16; row++) {
    uint16_t bits = ((uint16_t)buf[row * 2] << 8) | buf[row * 2 + 1];
    for (uint8_t col = 0; col < 16; col++) {
      tft.drawPixel(x + col, y + row, (bits & (0x8000 >> col)) ? fg : bg);
    }
  }
}

// 绘制 GB2312 字符串（汉字+ASCII 混排）
void drawChinese(int16_t x, int16_t y, const char* str, uint16_t fg, uint16_t bg) {
  int16_t cx = x;
  while (*str) {
    uint8_t ch = (uint8_t)*str;
    if (ch >= 0xA1 && *(str + 1)) {
      uint8_t gb[2] = { ch, (uint8_t)*(str + 1) };
      drawHanzi(cx, y, gb, fg, bg);
      cx  += 16;
      str += 2;
    } else {
      tft.drawChar(cx, y + 2, ch, fg, bg, 1);
      cx  += 6;
      str += 1;
    }
  }
}

// ── 传感器初始化 ─────────────────────────────
bool initGY91() {
  bool success = true;
  
  // 初始化 I2C（自定义引脚）
  Wire.begin(GY91_SDA, GY91_SCL);
  mpu.setWire(&Wire);
  
  // 初始化 MPU9250
  if (mpu.beginAccel() != 0) {
    Serial.println("[错误] MPU9250 加速度计初始化失败");
    success = false;
  }
  if (mpu.beginGyro() != 0) {
    Serial.println("[错误] MPU9250 陀螺仪初始化失败");
    success = false;
  }
  if (mpu.beginMag() != 0) {
    Serial.println("[警告] MPU9250 磁力计初始化失败（可能需要校准）");
    // 磁力计失败不视为整体失败
  }
  
  // 初始化 BMP280
  if (!bmp.begin(0x76)) {  // SDO=GND 时地址为 0x76
    Serial.println("[错误] BMP280 初始化失败");
    success = false;
  } else {
    // 配置 BMP280
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
  }
  
  return success;
}

// ── 读取传感器数据 ───────────────────────────
void readSensors() {
  // 读取加速度
  if (mpu.accelUpdate() == 0) {
    accelX = mpu.accelX();
    accelY = mpu.accelY();
    accelZ = mpu.accelZ();
  }
  
  // 读取陀螺仪
  if (mpu.gyroUpdate() == 0) {
    gyroX = mpu.gyroX();
    gyroY = mpu.gyroY();
    gyroZ = mpu.gyroZ();
  }
  
  // 读取磁力计
  if (mpu.magUpdate() == 0) {
    magX = mpu.magX();
    magY = mpu.magY();
    magZ = mpu.magZ();
  }
  
  // 读取 BMP280
  temperature = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0F;  // 转换为 hPa
  altitude = bmp.readAltitude(1013.25);    // 标准海平面气压
}

// ── 绘制数据到屏幕 ───────────────────────────
void drawData() {
  char buf[32];
  int16_t y = 0;
  
  // 标题
  drawChinese(4, y, "GY-91 传感器数据", COLOR_TITLE, COLOR_BG);
  y += 20;
  
  // 分隔线
  tft.drawFastHLine(0, y, 240, 0x8410);
  y += 4;
  
  // 加速度
  drawChinese(4, y, "加速度(g):", COLOR_ACCEL, COLOR_BG);
  y += 14;
  snprintf(buf, sizeof(buf), "X:%6.3f Y:%6.3f", accelX, accelY);
  tft.setCursor(4, y);
  tft.setTextColor(COLOR_VALUE);
  tft.print(buf);
  y += 12;
  snprintf(buf, sizeof(buf), "Z:%6.3f", accelZ);
  tft.setCursor(4, y);
  tft.print(buf);
  y += 16;
  
  // 陀螺仪
  drawChinese(4, y, "陀螺仪(dps):", COLOR_GYRO, COLOR_BG);
  y += 14;
  snprintf(buf, sizeof(buf), "X:%6.1f Y:%6.1f", gyroX, gyroY);
  tft.setCursor(4, y);
  tft.setTextColor(COLOR_VALUE);
  tft.print(buf);
  y += 12;
  snprintf(buf, sizeof(buf), "Z:%6.1f", gyroZ);
  tft.setCursor(4, y);
  tft.print(buf);
  y += 16;
  
  // 磁力计
  drawChinese(4, y, "磁力计(uT):", COLOR_MAG, COLOR_BG);
  y += 14;
  snprintf(buf, sizeof(buf), "X:%6.1f Y:%6.1f", magX, magY);
  tft.setCursor(4, y);
  tft.setTextColor(COLOR_VALUE);
  tft.print(buf);
  y += 12;
  snprintf(buf, sizeof(buf), "Z:%6.1f", magZ);
  tft.setCursor(4, y);
  tft.print(buf);
  y += 16;
  
  // 气压和温度
  drawChinese(4, y, "气压温度:", COLOR_PRESS, COLOR_BG);
  y += 14;
  snprintf(buf, sizeof(buf), "%.1fhPa %.1fC", pressure, temperature);
  tft.setCursor(4, y);
  tft.setTextColor(COLOR_VALUE);
  tft.print(buf);
  y += 12;
  snprintf(buf, sizeof(buf), "海拔:%.1fm", altitude);
  tft.setCursor(4, y);
  tft.print(buf);
}

// ── 绘制静态标签（减少刷新闪烁）───────────────
void drawStaticLabels() {
  tft.fillScreen(COLOR_BG);
  
  int16_t y = 0;
  
  // 标题
  drawChinese(4, y, "GY-91 传感器数据", COLOR_TITLE, COLOR_BG);
  y += 20;
  
  // 分隔线
  tft.drawFastHLine(0, y, 240, 0x8410);
  y += 4;
  
  // 加速度标签
  drawChinese(4, y, "加速度(g):", COLOR_ACCEL, COLOR_BG);
  y += 42;
  
  // 陀螺仪标签
  drawChinese(4, y, "陀螺仪(dps):", COLOR_GYRO, COLOR_BG);
  y += 42;
  
  // 磁力计标签
  drawChinese(4, y, "磁力计(uT):", COLOR_MAG, COLOR_BG);
  y += 42;
  
  // 气压温度标签
  drawChinese(4, y, "气压温度:", COLOR_PRESS, COLOR_BG);
}

// ── 只更新数值区域 ───────────────────────────
void updateValues() {
  char buf[32];
  int16_t y;
  
  // 加速度数值区域：y=24~52
  y = 24;
  tft.fillRect(4, y, 232, 26, COLOR_BG);
  snprintf(buf, sizeof(buf), "X:%6.3f Y:%6.3f", accelX, accelY);
  tft.setCursor(4, y);
  tft.setTextColor(COLOR_VALUE);
  tft.print(buf);
  y += 12;
  snprintf(buf, sizeof(buf), "Z:%6.3f", accelZ);
  tft.setCursor(4, y);
  tft.print(buf);
  
  // 陀螺仪数值区域：y=66~94
  y = 66;
  tft.fillRect(4, y, 232, 26, COLOR_BG);
  snprintf(buf, sizeof(buf), "X:%6.1f Y:%6.1f", gyroX, gyroY);
  tft.setCursor(4, y);
  tft.setTextColor(COLOR_VALUE);
  tft.print(buf);
  y += 12;
  snprintf(buf, sizeof(buf), "Z:%6.1f", gyroZ);
  tft.setCursor(4, y);
  tft.print(buf);
  
  // 磁力计数值区域：y=108~136
  y = 108;
  tft.fillRect(4, y, 232, 26, COLOR_BG);
  snprintf(buf, sizeof(buf), "X:%6.1f Y:%6.1f", magX, magY);
  tft.setCursor(4, y);
  tft.setTextColor(COLOR_VALUE);
  tft.print(buf);
  y += 12;
  snprintf(buf, sizeof(buf), "Z:%6.1f", magZ);
  tft.setCursor(4, y);
  tft.print(buf);
  
  // 气压温度数值区域：y=150~178
  y = 150;
  tft.fillRect(4, y, 232, 26, COLOR_BG);
  snprintf(buf, sizeof(buf), "%.1fhPa %.1fC", pressure, temperature);
  tft.setCursor(4, y);
  tft.setTextColor(COLOR_VALUE);
  tft.print(buf);
  y += 12;
  snprintf(buf, sizeof(buf), "海拔:%.1fm", altitude);
  tft.setCursor(4, y);
  tft.print(buf);
}

// ── Setup ────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n====================");
  Serial.println("GY-91 + ST7789 演示");
  Serial.println("====================\n");
  
  // 初始化背光和字库片选
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  
  // 初始化显示屏
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(COLOR_BG);
  tft.setTextSize(1);
  
  // 显示初始化信息
  drawChinese(4, 100, "正在初始化...", COLOR_TITLE, COLOR_BG);
  
  // 初始化 GY-91
  if (!initGY91()) {
    Serial.println("[错误] GY-91 初始化失败，请检查接线");
    tft.fillScreen(COLOR_BG);
    drawChinese(4, 100, "传感器初始化失败", 0xF800, COLOR_BG);
    drawChinese(4, 120, "请检查接线", 0xF800, COLOR_BG);
    while (1) { delay(100); }  // 停止运行
  }
  
  Serial.println("[成功] GY-91 初始化完成");
  
  // 绘制静态标签
  drawStaticLabels();
}

// ── Loop ─────────────────────────────────────
void loop() {
  // 读取传感器
  readSensors();
  
  // 更新显示（只刷新数值区域，减少闪烁）
  updateValues();
  
  // 串口输出（调试用）
  Serial.printf("Accel: %.3f, %.3f, %.3f | ", accelX, accelY, accelZ);
  Serial.printf("Gyro: %.1f, %.1f, %.1f | ", gyroX, gyroY, gyroZ);
  Serial.printf("Mag: %.1f, %.1f, %.1f | ", magX, magY, magZ);
  Serial.printf("Temp: %.1fC | Press: %.1fhPa\n", temperature, pressure);
  
  delay(100);  // 10Hz 刷新率
}
