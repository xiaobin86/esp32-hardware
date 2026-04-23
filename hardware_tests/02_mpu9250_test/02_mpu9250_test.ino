/**
 * ============================================
 * 02_mpu9250_test.ino
 * 四足机器马 - MPU9250 姿态传感器测试
 * ============================================
 * 
 * 功能说明：
 *   - 通过 I2C 读取 MPU9250 的加速度、陀螺仪、磁力计数据
 *   - 计算 Pitch（俯仰角）和 Roll（横滚角）
 *   - 串口输出原始数据与姿态角，观察传感器是否正常
 * 
 * 接线：
 *   MPU9250 VCC -> 3.3V
 *   MPU9250 GND -> GND
 *   MPU9250 SDA -> GPIO 3
 *   MPU9250 SCL -> GPIO 4
 *   MPU9250 INT -> 不接（本测试使用轮询方式）
 * 
 * 需要的库：
 *   - 无需额外库，使用 Arduino 内置 Wire 库
 * 
 * I2C 地址：
 *   - MPU9250: 0x68 (AD0 接 GND) 或 0x69 (AD0 接 VCC)
 *   - AK8963 磁力计: 0x0C
 */

#include <Wire.h>

// ============================================
// I2C 引脚与地址定义
// ============================================
#define I2C_SDA       3
#define I2C_SCL       4
#define MPU9250_ADDR  0x68   // MPU9250 I2C 地址（AD0 接 GND）
#define AK8963_ADDR   0x0C   // 磁力计 I2C 地址

// ============================================
// MPU9250 寄存器地址
// ============================================
#define REG_PWR_MGMT_1    0x6B
#define REG_CONFIG        0x1A
#define REG_GYRO_CONFIG   0x1B
#define REG_ACCEL_CONFIG  0x1C
#define REG_ACCEL_CONFIG2 0x1D
#define REG_INT_PIN_CFG   0x37
#define REG_ACCEL_XOUT_H  0x3B
#define REG_GYRO_XOUT_H   0x43
#define REG_WHO_AM_I      0x75

// 磁力计寄存器
#define AK8963_CNTL1      0x0A
#define AK8963_CNTL2      0x0B
#define AK8963_ASAX       0x10
#define AK8963_ST1        0x02
#define AK8963_XOUT_L     0x03

// ============================================
// 量程与灵敏度配置
// ============================================
// 加速度计量程: ±2g, 灵敏度: 16384 LSB/g
// 陀螺仪量程: ±250°/s, 灵敏度: 131 LSB/(°/s)
const float ACCEL_SCALE = 16384.0;
const float GYRO_SCALE  = 131.0;
const float MAG_SCALE   = 0.15;  // 磁力计 16bit 模式: 0.15 μT/LSB

// ============================================
// 数据变量
// ============================================
struct SensorData {
  int16_t ax, ay, az;   // 加速度原始值
  int16_t gx, gy, gz;   // 陀螺仪原始值
  int16_t mx, my, mz;   // 磁力计原始值
  
  float ax_g, ay_g, az_g;       // 加速度 (g)
  float gx_dps, gy_dps, gz_dps; // 角速度 (°/s)
  float mx_ut, my_ut, mz_ut;    // 磁场强度 (μT)
  
  float pitch;  // 俯仰角
  float roll;   // 横滚角
  float yaw;    // 航向角
} sensor;

// 磁力计校准系数（出厂校准）
float magScaleX = 1.0, magScaleY = 1.0, magScaleZ = 1.0;

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 200; // 输出间隔 200ms (5Hz)

// ============================================
// 初始化
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n============================================");
  Serial.println("   四足机器马 - MPU9250 姿态传感器测试");
  Serial.println("============================================");
  
  // 初始化 I2C（自定义引脚）
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);  // 400kHz 快速模式
  Serial.printf("\n[I2C] 已初始化: SDA=GPIO%d, SCL=GPIO%d, 速率=400kHz\n", I2C_SDA, I2C_SCL);
  
  // 检查 MPU9250 是否在线
  uint8_t whoami = readRegister(MPU9250_ADDR, REG_WHO_AM_I);
  Serial.printf("[检测] WHO_AM_I 寄存器 = 0x%02X (应为 0x71 或 0x73)\n", whoami);
  
  if (whoami != 0x71 && whoami != 0x73) {
    Serial.println("[错误] MPU9250 未检测到！请检查接线。");
    Serial.println("       常见原因：");
    Serial.println("       1. SDA/SCL 线接反或松动");
    Serial.println("       2. 电源未接好（需 3.3V，不是 5V）");
    Serial.println("       3. I2C 地址不对（检查 AD0 引脚）");
    while (1) { delay(100); }  // 停止运行
  }
  
  Serial.println("[成功] MPU9250 检测正常！\n");
  
  // 初始化 MPU9250
  initMPU9250();
  
  // 初始化磁力计
  initAK8963();
  
  Serial.println("\n[就绪] 传感器初始化完成！");
  Serial.println("[提示] 将 ESP32 放平，观察 Pitch/Roll 是否接近 0°");
  Serial.println("============================================\n");
  
  delay(1000);
}

// ============================================
// 主循环
// ============================================
void loop() {
  // 读取传感器数据
  readAccel();
  readGyro();
  readMagnetometer();
  
  // 计算姿态角
  calculateAttitude();
  
  // 定时输出
  unsigned long now = millis();
  if (now - lastPrintTime >= PRINT_INTERVAL) {
    lastPrintTime = now;
    printSensorData();
  }
  
  delay(10);  // 100Hz 采样
}

// ============================================
// 初始化 MPU9250
// ============================================
void initMPU9250() {
  Serial.println("[初始化] 配置 MPU9250...");
  
  // 唤醒芯片
  writeRegister(MPU9250_ADDR, REG_PWR_MGMT_1, 0x00);
  delay(100);
  
  // 配置低通滤波器
  writeRegister(MPU9250_ADDR, REG_CONFIG, 0x03);  // DLPF_CFG = 3 (41Hz)
  
  // 配置陀螺仪量程: ±250°/s
  writeRegister(MPU9250_ADDR, REG_GYRO_CONFIG, 0x00);
  
  // 配置加速度计量程: ±2g
  writeRegister(MPU9250_ADDR, REG_ACCEL_CONFIG, 0x00);
  
  // 配置加速度计低通滤波器
  writeRegister(MPU9250_ADDR, REG_ACCEL_CONFIG2, 0x03);
  
  // 开启旁路模式，使主 I2C 总线可以直接访问磁力计
  writeRegister(MPU9250_ADDR, REG_INT_PIN_CFG, 0x02);
  
  Serial.println("         陀螺仪量程: ±250°/s");
  Serial.println("         加速度计量程: ±2g");
  Serial.println("         低通滤波器: 41Hz");
}

// ============================================
// 初始化 AK8963 磁力计
// ============================================
void initAK8963() {
  Serial.println("[初始化] 配置 AK8963 磁力计...");
  
  // 复位磁力计
  writeRegister(AK8963_ADDR, AK8963_CNTL2, 0x01);
  delay(100);
  
  // 读取出厂校准系数
  uint8_t asax = readRegister(AK8963_ADDR, AK8963_ASAX);
  uint8_t asay = readRegister(AK8963_ADDR, AK8963_ASAX + 1);
  uint8_t asaz = readRegister(AK8963_ADDR, AK8963_ASAX + 2);
  
  magScaleX = ((asax - 128) * 0.5 / 128.0) + 1.0;
  magScaleY = ((asay - 128) * 0.5 / 128.0) + 1.0;
  magScaleZ = ((asaz - 128) * 0.5 / 128.0) + 1.0;
  
  Serial.printf("         校准系数: X=%.3f, Y=%.3f, Z=%.3f\n", magScaleX, magScaleY, magScaleZ);
  
  // 配置为连续测量模式 2 (100Hz)，16bit 输出
  writeRegister(AK8963_ADDR, AK8963_CNTL1, 0x16);
  delay(100);
  
  Serial.println("         测量模式: 连续 100Hz, 16bit");
}

// ============================================
// 读取加速度计
// ============================================
void readAccel() {
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU9250_ADDR, 6, true);
  
  sensor.ax = (Wire.read() << 8) | Wire.read();
  sensor.ay = (Wire.read() << 8) | Wire.read();
  sensor.az = (Wire.read() << 8) | Wire.read();
  
  sensor.ax_g = sensor.ax / ACCEL_SCALE;
  sensor.ay_g = sensor.ay / ACCEL_SCALE;
  sensor.az_g = sensor.az / ACCEL_SCALE;
}

// ============================================
// 读取陀螺仪
// ============================================
void readGyro() {
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(REG_GYRO_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU9250_ADDR, 6, true);
  
  sensor.gx = (Wire.read() << 8) | Wire.read();
  sensor.gy = (Wire.read() << 8) | Wire.read();
  sensor.gz = (Wire.read() << 8) | Wire.read();
  
  sensor.gx_dps = sensor.gx / GYRO_SCALE;
  sensor.gy_dps = sensor.gy / GYRO_SCALE;
  sensor.gz_dps = sensor.gz / GYRO_SCALE;
}

// ============================================
// 读取磁力计
// ============================================
void readMagnetometer() {
  // 检查数据是否就绪
  uint8_t st1 = readRegister(AK8963_ADDR, AK8963_ST1);
  if (!(st1 & 0x01)) return;  // 数据未就绪
  
  Wire.beginTransmission(AK8963_ADDR);
  Wire.write(AK8963_XOUT_L);
  Wire.endTransmission(false);
  Wire.requestFrom(AK8963_ADDR, 7, true);  // 6 bytes data + 1 byte ST2
  
  sensor.mx = (Wire.read() << 8) | Wire.read();
  sensor.my = (Wire.read() << 8) | Wire.read();
  sensor.mz = (Wire.read() << 8) | Wire.read();
  
  uint8_t st2 = Wire.read();  // 读取 ST2 以完成测量
  
  // 应用校准系数
  sensor.mx_ut = sensor.mx * magScaleX * MAG_SCALE;
  sensor.my_ut = sensor.my * magScaleY * MAG_SCALE;
  sensor.mz_ut = sensor.mz * magScaleZ * MAG_SCALE;
}

// ============================================
// 计算姿态角（简化版，仅使用加速度计）
// ============================================
void calculateAttitude() {
  // 使用加速度计计算 Pitch 和 Roll
  // 注意：此方法在剧烈运动时不准确，仅供参考
  sensor.pitch = atan2(sensor.ay_g, sqrt(sensor.ax_g * sensor.ax_g + sensor.az_g * sensor.az_g)) * 180.0 / PI;
  sensor.roll  = atan2(-sensor.ax_g, sensor.az_g) * 180.0 / PI;
  
  // 使用磁力计计算 Yaw（需先补偿 Pitch 和 Roll）
  // 简化计算，假设设备水平
  sensor.yaw = atan2(-sensor.my_ut, sensor.mx_ut) * 180.0 / PI;
  if (sensor.yaw < 0) sensor.yaw += 360.0;
}

// ============================================
// 串口输出数据
// ============================================
void printSensorData() {
  Serial.println("\n----- 传感器数据 -----");
  
  Serial.println("【加速度计】");
  Serial.printf("  原始值:  X=%6d  Y=%6d  Z=%6d\n", sensor.ax, sensor.ay, sensor.az);
  Serial.printf("  物理值:  X=%.3fg  Y=%.3fg  Z=%.3fg\n", sensor.ax_g, sensor.ay_g, sensor.az_g);
  
  Serial.println("【陀螺仪】");
  Serial.printf("  原始值:  X=%6d  Y=%6d  Z=%6d\n", sensor.gx, sensor.gy, sensor.gz);
  Serial.printf("  物理值:  X=%.2f°/s  Y=%.2f°/s  Z=%.2f°/s\n", sensor.gx_dps, sensor.gy_dps, sensor.gz_dps);
  
  Serial.println("【磁力计】");
  Serial.printf("  原始值:  X=%6d  Y=%6d  Z=%6d\n", sensor.mx, sensor.my, sensor.mz);
  Serial.printf("  物理值:  X=%.2fμT  Y=%.2fμT  Z=%.2fμT\n", sensor.mx_ut, sensor.my_ut, sensor.mz_ut);
  
  Serial.println("【姿态角】");
  Serial.printf("  Pitch(俯仰): %.2f°\n", sensor.pitch);
  Serial.printf("  Roll (横滚): %.2f°\n", sensor.roll);
  Serial.printf("  Yaw  (航向): %.2f°\n", sensor.yaw);
  
  Serial.println("----------------------");
}

// ============================================
// I2C 读写辅助函数
// ============================================
void writeRegister(uint8_t addr, uint8_t reg, uint8_t data) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(addr, 1, true);
  return Wire.read();
}
