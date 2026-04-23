/**
 * ============================================
 * 01_servo_test.ino
 * 四足机器马 - 4路舵机基础测试
 * ============================================
 * 
 * 功能说明：
 *   - 依次测试 4 条腿的舵机（左前、右前、左后、右后）
 *   - 每个舵机从 0° → 90° → 180° → 90° 摆动
 *   - 串口监视器输出当前角度，方便观察
 * 
 * 接线：
 *   舵机信号线(黄/橙) -> ESP32 GPIO
 *   舵机电源线(红)   -> 5V 外接电源正极
 *   舵机地线(棕/黑)  -> 5V 外接电源负极 + ESP32 GND（共地）
 * 
 * 重要提示：
 *   1. 舵机必须由独立 5V 电源供电，ESP32 的 5V 引脚无法驱动多个舵机
 *   2. ESP32 GND 必须与舵机电源负极相连（共地）
 *   3. 首次测试建议一次只接一个舵机，确认正常后再接多个
 */

#include <ESP32Servo.h>

// ============================================
// 引脚定义（根据 HARDWARE_SPEC.md 分配）
// ============================================
#define SERVO_LF  1   // 左前腿 (Left Front)
#define SERVO_RF  2   // 右前腿 (Right Front)
#define SERVO_LH  42  // 左后腿 (Left Hind)
#define SERVO_RH  41  // 右后腿 (Right Hind)

// ============================================
// 舵机对象
// ============================================
Servo servoLF;
Servo servoRF;
Servo servoLH;
Servo servoRH;

// 舵机角度限制（根据你的机械结构调整）
const int SERVO_MIN_ANGLE = 0;    // 最小角度
const int SERVO_MAX_ANGLE = 180;  // 最大角度
const int SERVO_CENTER    = 90;   // 中位（站立姿态）

// 测试参数
const int SWEEP_DELAY = 15;       // 角度变化间隔(ms)，越大越慢
const int PAUSE_TIME  = 1000;     // 每个位置停留时间(ms)

// ============================================
// 初始化
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n============================================");
  Serial.println("   四足机器马 - 舵机测试程序");
  Serial.println("============================================");
  Serial.println("\n[提示] 确保舵机电源已连接，且与 ESP32 共地!");
  Serial.println("[提示] 建议先只接一个舵机测试，逐步增加。\n");
  
  // ESP32Servo 库允许将 Servo 附加到任意 GPIO
  // 第二个参数是 PWM 通道（ESP32 有 16 个通道，0-15）
  servoLF.attach(SERVO_LF, 500, 2500);  // 500-2500us 脉宽对应 0-180°
  servoRF.attach(SERVO_RF, 500, 2500);
  servoLH.attach(SERVO_LH, 500, 2500);
  servoRH.attach(SERVO_RH, 500, 2500);
  
  Serial.println("[初始化] 4 路舵机已附加到 GPIO:");
  Serial.printf("         左前腿 -> GPIO %d\n", SERVO_LF);
  Serial.printf("         右前腿 -> GPIO %d\n", SERVO_RF);
  Serial.printf("         左后腿 -> GPIO %d\n", SERVO_LH);
  Serial.printf("         右后腿 -> GPIO %d\n", SERVO_RH);
  
  // 全部回到中位（90°）
  Serial.println("\n[动作] 所有舵机归中位 (90°)...");
  setAllServos(SERVO_CENTER);
  delay(2000);
  
  Serial.println("\n[就绪] 3 秒后开始测试...\n");
  delay(3000);
}

// ============================================
// 主循环 - 依次测试每个舵机
// ============================================
void loop() {
  // 测试顺序：左前 -> 右前 -> 左后 -> 右后
  testServo(servoLF, "左前腿", SERVO_LF);
  testServo(servoRF, "右前腿", SERVO_RF);
  testServo(servoLH, "左后腿", SERVO_LH);
  testServo(servoRH, "右后腿", SERVO_RH);
  
  // 全部一起摆动（模拟行走动作）
  Serial.println("\n[动作] 全部舵机同步摆动（模拟行走）...");
  allServoSwing();
  
  Serial.println("\n[完成] 本轮测试结束，3 秒后重新开始...");
  Serial.println("============================================\n");
  delay(3000);
}

// ============================================
// 单个舵机测试：0° -> 180° -> 0°
// ============================================
void testServo(Servo &servo, const char* name, int pin) {
  Serial.printf("\n[测试] %s (GPIO %d)\n", name, pin);
  Serial.println("       角度: 0° -> 180° -> 0°");
  
  // 0° -> 180°
  for (int angle = SERVO_MIN_ANGLE; angle <= SERVO_MAX_ANGLE; angle++) {
    servo.write(angle);
    if (angle % 30 == 0) {
      Serial.printf("       当前角度: %d°\n", angle);
    }
    delay(SWEEP_DELAY);
  }
  
  delay(PAUSE_TIME);
  Serial.println("       到达 180°，停顿 1 秒");
  
  // 180° -> 0°
  for (int angle = SERVO_MAX_ANGLE; angle >= SERVO_MIN_ANGLE; angle--) {
    servo.write(angle);
    if (angle % 30 == 0) {
      Serial.printf("       当前角度: %d°\n", angle);
    }
    delay(SWEEP_DELAY);
  }
  
  delay(PAUSE_TIME);
  Serial.println("       回到 0°，停顿 1 秒");
  
  // 回到中位
  servo.write(SERVO_CENTER);
  Serial.printf("       归中位 (%d°)\n", SERVO_CENTER);
  delay(500);
}

// ============================================
// 全部舵机同步摆动（模拟简单步态）
// ============================================
void allServoSwing() {
  // 对角腿同步摆动（小跑步态简化版）
  // 左前 + 右后 向前，右前 + 左后 向后
  
  Serial.println("       阶段1: 左前/右后 前摆，右前/左后 后摆");
  servoLF.write(120);
  servoRH.write(120);
  servoRF.write(60);
  servoLH.write(60);
  delay(500);
  
  Serial.println("       阶段2: 全部归中");
  setAllServos(SERVO_CENTER);
  delay(300);
  
  Serial.println("       阶段3: 左前/右后 后摆，右前/左后 前摆");
  servoLF.write(60);
  servoRH.write(60);
  servoRF.write(120);
  servoLH.write(120);
  delay(500);
  
  Serial.println("       阶段4: 全部归中");
  setAllServos(SERVO_CENTER);
  delay(300);
}

// ============================================
// 设置所有舵机到指定角度
// ============================================
void setAllServos(int angle) {
  servoLF.write(angle);
  servoRF.write(angle);
  servoLH.write(angle);
  servoRH.write(angle);
}
