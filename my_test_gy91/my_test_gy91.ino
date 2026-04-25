#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <Adafruit_BMP280.h>

// ========== 引脚定义 ==========
// I2C 引脚可以接任意 GPIO，这里用 GPIO17/16 作为示例
#define GY91_SDA  17
#define GY91_SCL  16

// ========== 数据结构 ==========
struct {
  float ax, ay, az;    // 加速度 (g)
  float gx, gy, gz;    // 陀螺仪 (°/s)
  float temp;          // 温度 (°C)
  float press;         // 气压 (hPa)
  float alt;           // 相对高度 (m)
} data;

// ========== 传感器对象 ==========
MPU9250_asukiaaa mpu;
Adafruit_BMP280 bmp;

// ========== 读取传感器 ==========
void readSensors() {
  // 读取加速度
  mpu.accelUpdate();
  data.ax = mpu.accelX();
  data.ay = mpu.accelY();
  data.az = mpu.accelZ();
  
  // 读取陀螺仪
  mpu.gyroUpdate();
  data.gx = mpu.gyroX();
  data.gy = mpu.gyroY();
  data.gz = mpu.gyroZ();
  
  // 读取气压和温度
  data.temp = bmp.readTemperature();
  data.press = bmp.readPressure() / 100.0F;  // Pa → hPa
  data.alt = bmp.readAltitude(1013.25F);      // 相对高度
}

// ========== 初始化 ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 1. 启动 I2C 总线
  Wire.begin(GY91_SDA, GY91_SCL);
  
  // 2. 初始化 MPU-6500
  mpu.setWire(&Wire);
  mpu.beginAccel();
  mpu.beginGyro();
  // 注意：MPU-6500 没有磁力计，不需要 beginMag()
  
  // 3. 初始化 BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 初始化失败！请检查接线");
    while (1);  // 停止运行
  }
  
  // 4. 配置 BMP280 采样参数
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,      // 正常工作模式
    Adafruit_BMP280::SAMPLING_X2,      // 温度采样 ×2
    Adafruit_BMP280::SAMPLING_X16,     // 气压采样 ×16
    Adafruit_BMP280::FILTER_X16,       // 滤波系数 ×16
    Adafruit_BMP280::STANDBY_MS_500    // 待机时间 500ms
  );
  
  Serial.println("GY-91 初始化成功！");
}

// ========== 主循环 ==========
void loop() {
  readSensors();
  
  Serial.printf("A:%.3f %.3f %.3f | ", data.ax, data.ay, data.az);
  Serial.printf("G:%.1f %.1f %.1f | ", data.gx, data.gy, data.gz);
  Serial.printf("T:%.1fC P:%.1fhPa A:%.1fm\n", data.temp, data.press, data.alt);
  
  delay(100);
}