/*
  ESP32-S3 + ST7789 + GY-91 (MPU9250 + BMP280)
  
  Hardware:
  - ESP32-S3-DevKitC-1
  - ST7789V 240x240 IPS Display (10Pin with font chip)
  - GY-91 10DOF Module (9-pin version, SAO/SDO shared pin)
  
  Wiring:
  ST7789 (J1 Left):
    VCC -> 3.3V  |  GND -> GND    |  BLK -> GPIO5
    DC  -> GPIO6  |  RES -> GPIO7  |  CS2 -> GPIO9 (font, unused here)
    CS1 -> GPIO10 |  SDA -> GPIO11 |  SCL -> GPIO12
    FSO -> GPIO13
    
  GY-91 (I2C):
    VIN -> 5V     |  GND -> GND    |  3V3 -> NC
    SCL -> GPIO4  |  SDA -> GPIO3  |  SAO/SDO -> GND
    NCS -> NC     |  CSB -> NC
    
  I2C Address: MPU9250=0x68, BMP280=0x76 (SAO/SDO=GND)
  
  Required Libraries:
  - Adafruit ST7789
  - Adafruit GFX Library
  - MPU9250_asukiaaa
  - Adafruit BMP280 Library
  - Adafruit Unified Sensor
*/
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <Adafruit_BMP280.h>
// ========== PIN DEFINITIONS ==========
// ST7789 Display (SPI)
#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL     5
#define FONT_CS    9
// GY-91 I2C (custom pins to avoid conflict with GPIO9 used by font chip)
#define GY91_SDA   3
#define GY91_SCL   4
// ========== COLOR DEFINITIONS (RGB565) ==========
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
// ========== GLOBAL OBJECTS ==========
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
MPU9250_asukiaaa mpu;
Adafruit_BMP280 bmp;
// ========== SENSOR DATA ==========
struct SensorData {
  float ax, ay, az;
  float gx, gy, gz;
  float mx, my, mz;
  float temp;
  float press;
  float alt;
  bool mpuReady = false;
  bool bmpReady = false;
} data;
// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 + ST7789 + GY-91 Demo");
  Serial.println("========================================");
  // Init backlight and font CS (keep font chip disabled)
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  // Init display
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(C_BLACK);
  tft.setTextWrap(false);
  // Show boot message
  drawBootScreen();
  // Init GY-91
  if (!initGY91()) {
    showError("Sensor Init Failed!", "Check GY-91 wiring");
    while (1) delay(100);
  }
  // Draw static UI layout
  drawLayout();
  
  Serial.println("[OK] All systems ready");
}
void drawBootScreen() {
  tft.setTextColor(C_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 80);
  tft.print("GY-91 Sensor");
  tft.setTextSize(1);
  tft.setCursor(40, 110);
  tft.setTextColor(C_CYAN);
  tft.print("Initializing...");
}
void showError(const char* line1, const char* line2) {
  tft.fillScreen(C_BLACK);
  tft.setTextColor(C_RED);
  tft.setTextSize(2);
  tft.setCursor(10, 90);
  tft.print(line1);
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE);
  tft.setCursor(30, 120);
  tft.print(line2);
  Serial.printf("[ERROR] %s - %s\n", line1, line2);
}
// ========== GY-91 INITIALIZATION ==========
bool initGY91() {
  bool ok = true;
  
  // Start I2C on custom pins (GPIO3/4)
  Wire.begin(GY91_SDA, GY91_SCL);
  mpu.setWire(&Wire);
  
  Serial.println("[INIT] MPU9250...");
  mpu.beginAccel();
  mpu.beginGyro();
  data.mpuReady = true;
  Serial.println("       Accel+Gyro OK");
  
  mpu.beginMag();
  Serial.println("       Magnetometer OK (calibrate if needed)");
  
  Serial.println("[INIT] BMP280...");
  if (bmp.begin(0x76)) {  // SDO=GND -> 0x76
    data.bmpReady = true;
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    Serial.println("       BMP280 OK");
  } else {
    Serial.println("       BMP280 FAILED");
    ok = false;
  }
  
  return ok;
}
// ========== READ SENSORS ==========
void readAllSensors() {
  if (data.mpuReady) {
    if (mpu.accelUpdate() == 0) {
      data.ax = mpu.accelX();
      data.ay = mpu.accelY();
      data.az = mpu.accelZ();
    }
    if (mpu.gyroUpdate() == 0) {
      data.gx = mpu.gyroX();
      data.gy = mpu.gyroY();
      data.gz = mpu.gyroZ();
    }
    if (mpu.magUpdate() == 0) {
      data.mx = mpu.magX();
      data.my = mpu.magY();
      data.mz = mpu.magZ();
    }
  }
  
  if (data.bmpReady) {
    data.temp = bmp.readTemperature();
    data.press = bmp.readPressure() / 100.0F;
    data.alt = bmp.readAltitude(1013.25F);
  }
}
// ========== DISPLAY LAYOUT ==========
void drawLayout() {
  tft.fillScreen(C_BLACK);
  
  // Title bar
  tft.fillRect(0, 0, 240, 22, 0x1082);  // Dark gray bar
  tft.setTextColor(C_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(6, 3);
  tft.print("GY-91 10DOF");
  
  // Section 1: Accelerometer
  drawSectionLabel(0, 24, "ACCEL (g)", C_CYAN);
  
  // Section 2: Gyroscope
  drawSectionLabel(0, 72, "GYRO (dps)", C_MAGENTA);
  
  // Section 3: Magnetometer
  drawSectionLabel(0, 120, "MAG (uT)", C_GREEN);
  
  // Section 4: Environment
  drawSectionLabel(0, 168, "ENV / ALT", C_ORANGE);
  
  // Footer line
  tft.drawFastHLine(0, 239, 240, C_GRAY);
}
void drawSectionLabel(uint8_t x, uint8_t y, const char* label, uint16_t color) {
  tft.fillRect(x, y, 240, 14, 0x0841);  // Dark background strip
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(x + 4, y + 3);
  tft.print(label);
}
// ========== UPDATE VALUES (partial refresh) ==========
void updateDisplay() {
  char buf[32];
  
  // --- ACCEL: y=40 ~ 66 ---
  fillValueArea(4, 40, 232, 24);
  tft.setCursor(4, 42);
  tft.setTextColor(C_WHITE);
  snprintf(buf, sizeof(buf), "X:%6.3f  Y:%6.3f", data.ax, data.ay);
  tft.print(buf);
  tft.setCursor(4, 54);
  snprintf(buf, sizeof(buf), "Z:%6.3f", data.az);
  tft.print(buf);
  
  // --- GYRO: y=88 ~ 114 ---
  fillValueArea(4, 88, 232, 24);
  tft.setCursor(4, 90);
  snprintf(buf, sizeof(buf), "X:%6.1f  Y:%6.1f", data.gx, data.gy);
  tft.print(buf);
  tft.setCursor(4, 102);
  snprintf(buf, sizeof(buf), "Z:%6.1f", data.gz);
  tft.print(buf);
  
  // --- MAG: y=136 ~ 162 ---
  fillValueArea(4, 136, 232, 24);
  tft.setCursor(4, 138);
  snprintf(buf, sizeof(buf), "X:%6.1f  Y:%6.1f", data.mx, data.my);
  tft.print(buf);
  tft.setCursor(4, 150);
  snprintf(buf, sizeof(buf), "Z:%6.1f", data.mz);
  tft.print(buf);
  
  // --- ENV: y=184 ~ 220 ---
  fillValueArea(4, 184, 232, 34);
  tft.setCursor(4, 186);
  snprintf(buf, sizeof(buf), "%.1f C   %.1f hPa", data.temp, data.press);
  tft.print(buf);
  tft.setCursor(4, 198);
  tft.setTextColor(C_BLUE);
  snprintf(buf, sizeof(buf), "ALT: %.1f m", data.alt);
  tft.print(buf);
  tft.setCursor(4, 212);
  tft.setTextColor(C_GRAY);
  tft.print("Ref: 1013.25 hPa");
}
inline void fillValueArea(int16_t x, int16_t y, int16_t w, int16_t h) {
  tft.fillRect(x, y, w, h, C_BLACK);
}
// ========== SERIAL OUTPUT ==========
void printSerial() {
  Serial.printf("[A] %.3f %.3f %.3f | ", data.ax, data.ay, data.az);
  Serial.printf("[G] %.1f %.1f %.1f | ", data.gx, data.gy, data.gz);
  Serial.printf("[M] %.1f %.1f %.1f | ", data.mx, data.my, data.mz);
  Serial.printf("[E] %.1fC %.1fhPa %.1fm\n", data.temp, data.press, data.alt);
}
// ========== MAIN LOOP ==========
void loop() {
  readAllSensors();
  updateDisplay();
  printSerial();
  delay(100);  // 10Hz update
}