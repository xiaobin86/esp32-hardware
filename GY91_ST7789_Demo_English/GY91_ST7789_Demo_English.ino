/**
 * ESP32-S3 + ST7789 + GY-91 (MPU9250 + BMP280) Demo
 * 
 * Function: Read accel/gyro/mag/pressure data and display on ST7789 screen
 * Language: English
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <Adafruit_BMP280.h>

// ── ST7789 Pin Definitions ──────────────────────────
#define TFT_CS    10   // Screen chip select
#define TFT_DC     6   // Data/command
#define TFT_RST    7   // Reset
#define TFT_MOSI  11   // SPI MOSI
#define TFT_SCLK  12   // SPI CLK
#define TFT_BL     5   // Backlight
#define TFT_MISO  13   // SPI MISO (for font chip)
#define FONT_CS    9   // Font chip select

// ── GY-91 I2C Pin Definitions ───────────────────────
#define GY91_SDA   3   // I2C Data
#define GY91_SCL   4   // I2C Clock

// ── Color Definitions (16-bit RGB565)───────────────────
#define COLOR_BG     0x0000  // Black background
#define COLOR_TITLE  0xFFE0  // Yellow title
#define COLOR_ACCEL  0x07FF  // Cyan - Accelerometer
#define COLOR_GYRO   0xF81F  // Purple - Gyroscope
#define COLOR_MAG    0x07E0  // Green - Magnetometer
#define COLOR_TEMP   0xFFA0  // Orange - Temperature
#define COLOR_PRESS  0x001F  // Blue - Pressure
#define COLOR_VALUE  0xFFFF  // White values

// ── Global Objects ─────────────────────────────────
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
MPU9250_asukiaaa mpu;
Adafruit_BMP280 bmp;

// ── Data Storage ─────────────────────────────────
float accelX, accelY, accelZ;
float gyroX, gyroY, gyroZ;
float magX, magY, magZ;
float temperature;
float pressure;
float altitude;

// ── Sensor Initialization ─────────────────────────────
bool initGY91() {
  bool success = true;
  
  // Initialize I2C (custom pins)
  Wire.begin(GY91_SDA, GY91_SCL);
  mpu.setWire(&Wire);
  
  // Initialize MPU9250
  if (mpu.beginAccel() != 0) {
    Serial.println("[Error] MPU9250 accelerometer init failed");
    success = false;
  }
  if (mpu.beginGyro() != 0) {
    Serial.println("[Error] MPU9250 gyroscope init failed");
    success = false;
  }
  if (mpu.beginMag() != 0) {
    Serial.println("[Warning] MPU9250 magnetometer init failed (may need calibration)");
    // Magnetometer failure not treated as overall failure
  }
  
  // Initialize BMP280
  if (!bmp.begin(0x76)) {  // Address 0x76 when SDO=GND
    Serial.println("[Error] BMP280 init failed");
    success = false;
  } else {
    // Configure BMP280
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
  }
  
  return success;
}

// ── Read Sensor Data ───────────────────────────
void readSensors() {
  // Read accelerometer
  if (mpu.accelUpdate() == 0) {
    accelX = mpu.accelX();
    accelY = mpu.accelY();
    accelZ = mpu.accelZ();
  }
  
  // Read gyroscope
  if (mpu.gyroUpdate() == 0) {
    gyroX = mpu.gyroX();
    gyroY = mpu.gyroY();
    gyroZ = mpu.gyroZ();
  }
  
  // Read magnetometer
  if (mpu.magUpdate() == 0) {
    magX = mpu.magX();
    magY = mpu.magY();
    magZ = mpu.magZ();
  }
  
  // Read BMP280
  temperature = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0F;  // Convert to hPa
  altitude = bmp.readAltitude(1013.25);    // Standard sea level pressure
}

// ── Draw Static Labels (reduce flicker)───────────────
void drawStaticLabels() {
  tft.fillScreen(COLOR_BG);
  
  // Title
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(2);
  tft.setCursor(4, 0);
  tft.println("GY-91 Sensor");
  
  // Divider line
  tft.drawFastHLine(0, 20, 240, 0x8410);
  
  int16_t y = 24;
  
  // Accelerometer label
  tft.setTextColor(COLOR_ACCEL);
  tft.setTextSize(1);
  tft.setCursor(4, y);
  tft.print("ACCEL (g)");
  y += 42;
  
  // Gyroscope label
  tft.setTextColor(COLOR_GYRO);
  tft.setCursor(4, y);
  tft.print("GYRO (dps)");
  y += 42;
  
  // Magnetometer label
  tft.setTextColor(COLOR_MAG);
  tft.setCursor(4, y);
  tft.print("MAG (uT)");
  y += 42;
  
  // Pressure/Temp label
  tft.setTextColor(COLOR_PRESS);
  tft.setCursor(4, y);
  tft.print("ENV");
}

// ── Update Value Areas Only ───────────────────────────
void updateValues() {
  char buf[32];
  
  // Accelerometer values: y=36~58
  tft.fillRect(4, 36, 232, 22, COLOR_BG);
  tft.setTextColor(COLOR_VALUE);
  tft.setTextSize(1);
  tft.setCursor(4, 36);
  snprintf(buf, sizeof(buf), "X:%6.3f Y:%6.3f", accelX, accelY);
  tft.print(buf);
  tft.setCursor(4, 48);
  snprintf(buf, sizeof(buf), "Z:%6.3f", accelZ);
  tft.print(buf);
  
  // Gyroscope values: y=78~100
  tft.fillRect(4, 78, 232, 22, COLOR_BG);
  tft.setCursor(4, 78);
  snprintf(buf, sizeof(buf), "X:%6.1f Y:%6.1f", gyroX, gyroY);
  tft.print(buf);
  tft.setCursor(4, 90);
  snprintf(buf, sizeof(buf), "Z:%6.1f", gyroZ);
  tft.print(buf);
  
  // Magnetometer values: y=120~142
  tft.fillRect(4, 120, 232, 22, COLOR_BG);
  tft.setCursor(4, 120);
  snprintf(buf, sizeof(buf), "X:%6.1f Y:%6.1f", magX, magY);
  tft.print(buf);
  tft.setCursor(4, 132);
  snprintf(buf, sizeof(buf), "Z:%6.1f", magZ);
  tft.print(buf);
  
  // Environment values: y=162~184
  tft.fillRect(4, 162, 232, 22, COLOR_BG);
  tft.setCursor(4, 162);
  snprintf(buf, sizeof(buf), "%.1fhPa %.1fC", pressure, temperature);
  tft.print(buf);
  tft.setCursor(4, 174);
  snprintf(buf, sizeof(buf), "Alt:%.1fm", altitude);
  tft.print(buf);
}

// ── Setup ────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n====================");
  Serial.println("GY-91 + ST7789 Demo");
  Serial.println("====================\n");
  
  // Initialize backlight and font chip select
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);
  
  // Initialize display
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(COLOR_BG);
  tft.setTextSize(1);
  
  // Show init message
  tft.setTextColor(COLOR_TITLE);
  tft.setCursor(4, 100);
  tft.print("Initializing...");
  
  // Initialize GY-91
  if (!initGY91()) {
    Serial.println("[Error] GY-91 init failed, check wiring");
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(0xF800);
    tft.setCursor(4, 100);
    tft.print("Sensor Init Failed!");
    tft.setCursor(4, 120);
    tft.print("Check Wiring");
    while (1) { delay(100); }  // Stop
  }
  
  Serial.println("[OK] GY-91 initialized");
  
  // Draw static labels
  drawStaticLabels();
}

// ── Loop ─────────────────────────────────────
void loop() {
  // Read sensors
  readSensors();
  
  // Update display (only value areas to reduce flicker)
  updateValues();
  
  // Serial output (for debugging)
  Serial.printf("Accel: %.3f, %.3f, %.3f | ", accelX, accelY, accelZ);
  Serial.printf("Gyro: %.1f, %.1f, %.1f | ", gyroX, gyroY, gyroZ);
  Serial.printf("Mag: %.1f, %.1f, %.1f | ", magX, magY, magZ);
  Serial.printf("Temp: %.1fC | Press: %.1fhPa\n", temperature, pressure);
  
  delay(100);  // 10Hz refresh rate
}
