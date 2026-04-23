#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include "config.h"

// ST7789 Pins
#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL     5
#define FONT_CS    9

// GY-91 I2C Pins
#define GY91_SDA   3
#define GY91_SCL   4

// WiFi credentials loaded from config.h (not tracked by git)

// Colors
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_YELLOW  0xFFE0
#define C_CYAN    0x07FF
#define C_MAGENTA 0xF81F
#define C_GREEN   0x07E0
#define C_ORANGE  0xFC00
#define C_BLUE    0x001F
#define C_GRAY    0x8410
#define C_RED     0xF800

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
MPU9250_asukiaaa mpu;
Adafruit_BMP280 bmp;

struct {
  float ax, ay, az;
  float gx, gy, gz;
  float mx, my, mz;
  float temp, press, alt;
} data;

bool initGY91() {
  Wire.begin(GY91_SDA, GY91_SCL);
  mpu.setWire(&Wire);
  mpu.beginAccel();
  mpu.beginGyro();
  mpu.beginMag();
  
  if (!bmp.begin(0x76)) return false;
  
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  return true;
}

void readSensors() {
  mpu.accelUpdate();
  data.ax = mpu.accelX();
  data.ay = mpu.accelY();
  data.az = mpu.accelZ();
  
  mpu.gyroUpdate();
  data.gx = mpu.gyroX();
  data.gy = mpu.gyroY();
  data.gz = mpu.gyroZ();
  
  mpu.magUpdate();
  data.mx = mpu.magX();
  data.my = mpu.magY();
  data.mz = mpu.magZ();
  
  data.temp = bmp.readTemperature();
  data.press = bmp.readPressure() / 100.0F;
  data.alt = bmp.readAltitude(1013.25F);
}

void drawLayout() {
  tft.fillScreen(C_BLACK);
  
  // Title bar
  tft.fillRect(0, 0, 240, 22, 0x1082);
  tft.setTextColor(C_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(6, 3);
  tft.print("GY-91 10DOF");
  
  // Labels
  drawLabel(0, 24, "ACCEL (g)", C_CYAN);
  drawLabel(0, 72, "GYRO (dps)", C_MAGENTA);
  drawLabel(0, 120, "MAG (uT)", C_GREEN);
  drawLabel(0, 168, "ENV / ALT", C_ORANGE);
}

void drawLabel(uint8_t x, uint8_t y, const char* label, uint16_t color) {
  tft.fillRect(x, y, 240, 14, 0x0841);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(x + 4, y + 3);
  tft.print(label);
}

void updateValues() {
  char buf[32];
  
  tft.fillRect(4, 40, 232, 24, C_BLACK);
  tft.setCursor(4, 42);
  tft.setTextColor(C_WHITE);
  snprintf(buf, sizeof(buf), "X:%6.3f  Y:%6.3f", data.ax, data.ay);
  tft.print(buf);
  tft.setCursor(4, 54);
  snprintf(buf, sizeof(buf), "Z:%6.3f", data.az);
  tft.print(buf);
  
  tft.fillRect(4, 88, 232, 24, C_BLACK);
  tft.setCursor(4, 90);
  snprintf(buf, sizeof(buf), "X:%6.1f  Y:%6.1f", data.gx, data.gy);
  tft.print(buf);
  tft.setCursor(4, 102);
  snprintf(buf, sizeof(buf), "Z:%6.1f", data.gz);
  tft.print(buf);
  
  tft.fillRect(4, 136, 232, 24, C_BLACK);
  tft.setCursor(4, 138);
  snprintf(buf, sizeof(buf), "X:%6.1f  Y:%6.1f", data.mx, data.my);
  tft.print(buf);
  tft.setCursor(4, 150);
  snprintf(buf, sizeof(buf), "Z:%6.1f", data.mz);
  tft.print(buf);
  
  tft.fillRect(4, 184, 232, 34, C_BLACK);
  tft.setCursor(4, 186);
  snprintf(buf, sizeof(buf), "%.1f C   %.1f hPa", data.temp, data.press);
  tft.print(buf);
  tft.setCursor(4, 198);
  tft.setTextColor(C_BLUE);
  snprintf(buf, sizeof(buf), "ALT: %.1f m", data.alt);
  tft.print(buf);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  //init wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.println("connect...");
  // 等待连接，最多尝试20秒
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi 连接成功！");
    Serial.print("IP 地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi 连接失败，请检查名称和密码");
  }
  
  
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(C_BLACK);
  
  tft.setTextColor(C_CYAN);
  tft.setCursor(40, 110);
  tft.print("Initializing...");
  
  if (!initGY91()) {
    tft.fillScreen(C_BLACK);
    tft.setTextColor(C_RED);
    tft.setCursor(10, 100);
    tft.print("Sensor Init Failed!");
    while (1) delay(100);
  }
  
  drawLayout();
}

void loop() {
  // 可以在这里检查连接状态
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi 断开，尝试重连...");
    WiFi.reconnect();
  }
  readSensors();
  updateValues();
  
  Serial.printf("A:%.3f %.3f %.3f | ", data.ax, data.ay, data.az);
  Serial.printf("G:%.1f %.1f %.1f | ", data.gx, data.gy, data.gz);
  Serial.printf("M:%.1f %.1f %.1f | ", data.mx, data.my, data.mz);
  Serial.printf("T:%.1fC P:%.1fhPa\n", data.temp, data.press);
  
  delay(100);
}