# 四足机器马 - 硬件测试代码

本文件夹包含所有硬件模块的独立测试代码，建议按编号顺序逐一测试，确认每个模块正常工作后再进行整合。

---

## 目录结构

```
hardware_tests/
├── 01_servo_test/           # 舵机测试
│   └── 01_servo_test.ino
├── 02_mpu9250_test/         # 姿态传感器测试
│   └── 02_mpu9250_test.ino
├── 03_display_test/         # 显示屏测试
│   └── 03_display_test.ino
├── 04_camera_test/          # 摄像头测试
│   └── 04_camera_test.ino
├── wiring_diagram.png       # 接线图（Excalidraw 格式）
└── README.md                # 本文件
```

---

## 快速开始

### 1. Arduino IDE 环境准备

1. 安装 **Arduino IDE**（建议 2.0 以上版本）
2. 添加 ESP32 开发板支持：
   - 文件 → 首选项 → 附加开发板管理器网址，添加：
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - 工具 → 开发板 → 开发板管理器 → 搜索 **esp32** → 安装 **ESP32 by Espressif Systems**
3. 选择开发板：
   - 工具 → 开发板 → **ESP32S3 Dev Module**
   - 工具 → Flash Mode → **QIO 80MHz**
   - 工具 → Flash Size → **16MB (128Mb)**
   - 工具 → Partition Scheme → **Huge APP (3MB No OTA/1MB SPIFFS)**
   - 工具 → PSRAM → **OPI PSRAM**

### 2. 安装必要的库

在 Arduino IDE 中：
- **项目 → 加载库 → 管理库**，搜索并安装：
  - `ESP32Servo` by Kevin Harrington, John K. Bennett
  - `TFT_eSPI` by Bodmer
  - `esp32-camera`（需从 GitHub 手动安装）

**esp32-camera 手动安装步骤：**
1. 下载 https://github.com/espressif/esp32-camera/archive/refs/heads/master.zip
2. 解压到 Arduino 库文件夹：`文档/Arduino/libraries/esp32-camera/`
3. 重启 Arduino IDE

### 3. TFT_eSPI 库配置（重要！）

打开 TFT_eSPI 库的配置文件：
- Windows: `文档/Arduino/libraries/TFT_eSPI/User_Setup.h`

**取消注释（启用）以下配置：**

```cpp
// 选择你的显示屏驱动（二选一）
#define ST7789_DRIVER     // 如果使用 ST7789
// #define ILI9341_DRIVER // 如果使用 ILI9341

// 屏幕分辨率
#define TFT_WIDTH  240
#define TFT_HEIGHT 240    // ST7789 方形屏
// #define TFT_HEIGHT 320 // ILI9341 或 ST7789 长屏

// 引脚定义
#define TFT_MOSI 35
#define TFT_SCLK 36
#define TFT_CS   34
#define TFT_DC   33
#define TFT_RST  38

// 加载字体
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

// 其他选项
#define SMOOTH_FONT
```

**注意：** 编辑前建议备份原文件。

---

## 测试顺序与使用方法

### 测试 1：舵机测试（最简单，推荐第一个做）

**接线：**
| 舵机 | 信号线 | 电源线 | 地线 |
|------|--------|--------|------|
| 左前腿 | GPIO 1 | 5V 外接电源 | GND（共地）|
| 右前腿 | GPIO 2 | 5V 外接电源 | GND（共地）|
| 左后腿 | GPIO 42 | 5V 外接电源 | GND（共地）|
| 右后腿 | GPIO 41 | 5V 外接电源 | GND（共地）|

**重要：**
- ESP32 GND 必须与舵机电源负极相连（共地）
- 舵机电源必须是独立的 5V/3A 以上电源，ESP32 的 5V 引脚带不动舵机
- **首次测试建议只接一个舵机**

**步骤：**
1. 用 Arduino IDE 打开 `01_servo_test/01_servo_test.ino`
2. 连接 USB 线到 ESP32
3. 选择正确的 COM 端口（工具 → 端口）
4. 点击上传按钮
5. 上传完成后，打开 **工具 → 串口监视器**，波特率设为 **115200**
6. 观察串口输出，确认每个舵机依次摆动

**预期现象：**
- 串口显示初始化信息
- 左前腿舵机先摆动 0°→180°→0°
- 然后右前腿、左后腿、右后腿依次摆动
- 最后四个舵机同步模拟行走动作

---

### 测试 2：MPU9250 姿态传感器测试

**接线：**
| MPU9250 | ESP32-S3 |
|---------|----------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 3 |
| SCL | GPIO 4 |
| INT | 不接 |

**步骤：**
1. 打开 `02_mpu9250_test/02_mpu9250_test.ino`
2. 上传代码
3. 打开串口监视器（115200 波特率）
4. 观察输出数据

**预期现象：**
- 串口显示 "MPU9250 检测正常"
- 每 200ms 输出一组数据：
  - 加速度计 X/Y/Z（g）
  - 陀螺仪 X/Y/Z（°/s）
  - 磁力计 X/Y/Z（μT）
  - Pitch（俯仰角）、Roll（横滚角）、Yaw（航向角）
- 将 ESP32 放平，Pitch 和 Roll 应接近 0°
- 倾斜 ESP32，角度值应随之变化

**常见问题：**
- 如果显示 "MPU9250 未检测到"，检查 SDA/SCL 是否接反，或 I2C 地址是否正确（AD0 引脚接 GND 是 0x68，接 VCC 是 0x69）

---

### 测试 3：SPI 显示屏测试

**接线：**
| 显示屏 | ESP32-S3 |
|--------|----------|
| VCC | 3.3V |
| GND | GND |
| SCL/SCK | GPIO 36 |
| SDA/MOSI | GPIO 35 |
| RES/RST | GPIO 38 |
| DC | GPIO 33 |
| CS | GPIO 34 |
| BLK/LED | GPIO 37（或接 3.3V 常亮）|

**步骤：**
1. **先完成 TFT_eSPI 库配置（见上文）**
2. 打开 `03_display_test/03_display_test.ino`
3. 上传代码
4. 观察显示屏

**预期现象：**
- 屏幕显示绿色标题栏 "姿态监控"
- 显示模拟的传感器数据（加速度、陀螺仪、磁力计）
- 显示模拟的姿态角（Pitch、Roll、Yaw）
- 底部显示一个圆点，模拟姿态倾斜可视化
- 数据每 100ms 更新一次

**常见问题：**
- 屏幕白屏/花屏：检查 TFT_eSPI 的 `User_Setup.h` 配置，确认驱动型号和引脚正确
- 颜色异常：可能是 RGB/BGR 顺序问题，在 User_Setup.h 中尝试添加/取消注释 `#define TFT_RGB_ORDER TFT_BGR`
- 显示方向不对：修改 `tft.setRotation(0)` 中的参数（0-3）

---

### 测试 4：OV2640 摄像头测试

**接线（DVP 24Pin）：**
| 摄像头 | ESP32-S3 |
|--------|----------|
| 3.3V | 3.3V |
| GND | GND |
| D0 | GPIO 11 |
| D1 | GPIO 13 |
| D2 | GPIO 15 |
| D3 | GPIO 16 |
| D4 | GPIO 17 |
| D5 | GPIO 18 |
| D6 | GPIO 19 |
| D7 | GPIO 20 |
| PCLK | GPIO 12 |
| VSYNC | GPIO 14 |
| HREF | GPIO 21 |
| XCLK | GPIO 10 |
| SDA | GPIO 8 |
| SCL | GPIO 9 |
| PWDN | GPIO 7（或接 GND）|
| RESET | GPIO 6（或接 3.3V）|

**步骤：**
1. **修改代码中的 WiFi 信息：**
   ```cpp
   const char* ssid = "你的WiFi名称";
   const char* password = "你的WiFi密码";
   ```
2. 打开 `04_camera_test/04_camera_test.ino`
3. 上传代码
4. 打开串口监视器（115200 波特率）
5. 等待 WiFi 连接成功，记录 IP 地址
6. 手机或电脑连接同一 WiFi，浏览器访问 `http://<IP地址>/`
7. 点击"开始视频流"查看实时画面

**预期现象：**
- 串口显示摄像头初始化成功
- 串口显示 ESP32 的 IP 地址
- 浏览器打开页面后，能看到实时视频流
- 点击"拍照"可查看单张图片

**常见问题：**
- 摄像头初始化失败：检查 PSRAM 是否启用（Arduino IDE 工具菜单中设置）
- 画面花屏/偏色：检查 D0-D7 接线是否按顺序正确连接
- WiFi 连接失败：确认 WiFi 名称和密码正确，ESP32-S3 只支持 2.4GHz WiFi

---

## 完整接线汇总表

### ESP32-S3 引脚分配总览

| GPIO | 功能 | 连接设备 |
|------|------|----------|
| 1 | PWM | 左前腿舵机信号 |
| 2 | PWM | 右前腿舵机信号 |
| 3 | I2C SDA | MPU9250 SDA |
| 4 | I2C SCL | MPU9250 SCL |
| 6 | GPIO | 摄像头 RESET |
| 7 | GPIO | 摄像头 PWDN |
| 8 | I2C SDA | 摄像头 SDA |
| 9 | I2C SCL | 摄像头 SCL |
| 10 | XCLK | 摄像头 XCLK |
| 11 | D0 | 摄像头 D0 |
| 12 | PCLK | 摄像头 PCLK |
| 13 | D1 | 摄像头 D1 |
| 14 | VSYNC | 摄像头 VSYNC |
| 15 | D2 | 摄像头 D2 |
| 16 | D3 | 摄像头 D3 |
| 17 | D4 | 摄像头 D4 |
| 18 | D5 | 摄像头 D5 |
| 19 | D6 | 摄像头 D6 |
| 20 | D7 | 摄像头 D7 |
| 21 | HREF | 摄像头 HREF |
| 33 | DC | 显示屏 DC |
| 34 | CS | 显示屏 CS |
| 35 | MOSI | 显示屏 MOSI |
| 36 | SCK | 显示屏 SCK |
| 37 | BLK | 显示屏背光 |
| 38 | RST | 显示屏 RST |
| 41 | PWM | 右后腿舵机信号 |
| 42 | PWM | 左后腿舵机信号 |

---

## 安全注意事项

1. **电源隔离**：舵机必须使用独立电源，切勿将多个舵机直接接到 ESP32 的 5V 引脚
2. **共地连接**：ESP32 GND 必须与舵机电源负极相连，否则舵机不受控
3. **电压注意**：MPU9250、显示屏、摄像头都是 **3.3V** 供电，**切勿接 5V**
4. **首次测试**：每个模块首次测试时建议单独接线，确认正常后再组合
5. **USB 供电**：测试单个模块时可以用 USB 供电，但运行多个舵机时必须使用外部电源

---

## 下一步

所有硬件测试通过后，可以参考项目根目录的 `HARDWARE_SPEC.md`，开始编写整合代码，实现四足机器马的运动控制。

如有问题，可以查看各测试代码文件顶部的详细注释。
