# 我的硬件清单

> 本文档记录我**实际拥有**的硬件设备。以后所有接线图、示例代码、项目开发均基于这些硬件。
> 
> **规则**：当我第一次提到某个新硬件/芯片时，AI 应该**主动询问**我是否已购买该硬件，并根据我的回答更新此清单。
> 
> **参考知识库**：`D:\acela\my_wiki\我的知识库\raw\esp32-hardware\` 存放了各硬件的接线文档和示例代码，可供后续项目参考。

---

## 开发环境

- **IDE**: Arduino IDE
- **主板支持包**: ESP32 (by Espressif Systems)

---

## 已拥有硬件

### 1. 主控板
- **ESP32-S3-DevKitC-1**
  - 型号：ESP32-S3-DevKitC-1-N16R8
  - 核心：ESP32-S3 (Xtensa LX7 双核 @ 240MHz)
  - Flash：16MB
  - PSRAM：8MB
  - 无线：Wi-Fi 4 + 蓝牙 5.0
  - 工作电压：3.3V（USB 供电 5V）
  - 开发板排针：J1（左侧）、J3（右侧）
  
  **官方引脚布局参考**：[ESP32-S3-DevKitC-1 v1.1 排针](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)
  
  **J1 左侧（从上到下 1~22）**：
  | 序号 | 名称 | 功能 |
  |------|------|------|
  | 1 | 3V3 | 3.3V 电源 |
  | 2 | 3V3 | 3.3V 电源 |
  | 3 | RST | EN（复位）|
  | 4 | **GPIO4** | RTC_GPIO4, TOUCH4, ADC1_CH3 |
  | 5 | **GPIO5** | RTC_GPIO5, TOUCH5, ADC1_CH4 |
  | 6 | **GPIO6** | RTC_GPIO6, TOUCH6, ADC1_CH5 |
  | 7 | **GPIO7** | RTC_GPIO7, TOUCH7, ADC1_CH6 |
  | 8 | GPIO15 | RTC_GPIO15, U0RTS, ADC2_CH4 |
  | 9 | GPIO16 | RTC_GPIO16, U0CTS, ADC2_CH5 |
  | 10 | GPIO17 | RTC_GPIO17, U1TXD, ADC2_CH6 |
  | 11 | GPIO18 | RTC_GPIO18, U1RXD, ADC2_CH7 |
  | 12 | GPIO8 | RTC_GPIO8, TOUCH8, ADC1_CH7 |
  | 13 | **GPIO3** | RTC_GPIO3, TOUCH3, ADC1_CH2 |
  | 14 | GPIO46 | GPIO46 |
  | 15 | **GPIO9** | RTC_GPIO9, TOUCH9, ADC1_CH8, FSPIHD |
  | 16 | **GPIO10** | RTC_GPIO10, TOUCH10, ADC1_CH9, FSPICS0 |
  | 17 | **GPIO11** | RTC_GPIO11, TOUCH11, ADC2_CH0, FSPID |
  | 18 | **GPIO12** | RTC_GPIO12, TOUCH12, ADC2_CH1, FSPICLK |
  | 19 | **GPIO13** | RTC_GPIO13, TOUCH13, ADC2_CH2, FSPIQ |
  | 20 | GPIO14 | RTC_GPIO14, TOUCH14, ADC2_CH3, FSPIWP |
  | 21 | 5V | 5V 电源 |
  | 22 | GND | 接地 |
  
  **J3 右侧（从上到下 1~22）**：
  | 序号 | 名称 | 功能 |
  |------|------|------|
  | 1 | GND | 接地 |
  | 2 | TX | U0TXD, GPIO43 |
  | 3 | RX | U0RXD, GPIO44 |
  | 4 | **GPIO1** | RTC_GPIO1, TOUCH1, ADC1_CH0 |
  | 5 | GPIO2 | RTC_GPIO2, TOUCH2, ADC1_CH1 |
  | 6 | GPIO42 | MTMS, GPIO42 |
  | 7 | GPIO41 | MTDI, GPIO41 |
  | 8 | GPIO40 | MTDO, GPIO40 |
  | 9 | GPIO39 | MTCK, GPIO39 |
  | 10 | GPIO38 | GPIO38, RGB LED |
  | 11 | GPIO37 | SPIDQS, GPIO37 |
  | 12 | GPIO36 | SPIIO7, GPIO36 |
  | 13 | GPIO35 | SPIIO6, GPIO35 |
  | 14 | GPIO0 | RTC_GPIO0 |
  | 15 | GPIO45 | GPIO45 |
  | 16 | GPIO48 | GPIO48 |
  | 17 | **GPIO47** | GPIO47 |
  | 18 | **GPIO21** | RTC_GPIO21 |
  | 19 | GPIO20 | RTC_GPIO20, USB_D+ |
  | 20 | GPIO19 | RTC_GPIO19, USB_D- |
  | 21 | GND | 接地 |
  | 22 | GND | 接地 |

### 2. 显示屏
- **ST7789V 彩色显示屏（带字库版，10Pin）**
  - 驱动芯片：ST7789V
  - 字库芯片：GT30L32S4W（GB2312 编码，16×16 点阵）
  - 分辨率：240×240
  - 颜色：彩色 IPS
  - 接口：SPI（FSPI）
  - 引脚数：10 个针脚版本
  - 背光：支持 PWM 控制
  
  **引脚定义**（接 ESP32-S3 J1 左侧）：

  模块物理布局（上下两排，共10针）：
  ```
  上排: FSO    CS1    RES    SCL    GND
  下排: CS2    BLK    DC     SDA    VCC
  ```

  | 显示屏 | 接 GPIO | J1 位置 | 功能说明 |
  |--------|---------|---------|----------|
  | VCC | 3.3V | J1-1 | 电源 |
  | GND | GND | J1-22 | 地线 |
  | BLK | GPIO5 | J1-5 | 背光控制 |
  | DC | GPIO6 | J1-6 | 数据/命令选择 |
  | RES | GPIO7 | J1-7 | 复位 |
  | CS2 | GPIO9 | J1-15 | 字库芯片片选 |
  | CS1 | GPIO10 | J1-16 | 显示屏片选 |
  | SDA | GPIO11 | J1-17 | SPI MOSI（数据）|
  | SCL | GPIO12 | J1-18 | SPI CLK（时钟）|
  | FSO | GPIO13 | J1-19 | SPI MISO（字库回读）|

  **关键特性**：
  - SCL/SDA 屏和字库共用，通过 CS1/CS2 分时片选
  - 字库地址：16×16 点阵从 `0x2C9D0` 开始
  - 代码需以 **GB2312 编码**保存才能正确显示中文

### 3. 音频功放模块
- **MAX98357A I2S 功放模块**
  - 型号：MAX98357AETE
  - 类型：I2S 数字输入 D 类功放
  - 输出：单声道，最大 3W
  - 供电：3.3V ~ 5V（5V 音量更大）
  - 内置 DAC：直接转换 I2S 数字信号，无需外接 DAC
  
  **物理布局**（从左到右，共7针）：
  ```
  LRC    BCLK    DIN    GAIN    SD    GND    VIN
  ```

  **引脚定义**（接 ESP32-S3 J3 右侧，与显示屏隔离）：
  | 功放 | 接 GPIO | J3 位置 | 功能说明 |
  |------|---------|---------|----------|
  | LRC | GPIO21 | J3-18 | I2S 声道时钟（WS/LRCLK）|
  | BCLK | GPIO47 | J3-17 | I2S 位时钟 |
  | DIN | GPIO1 | J3-4 | I2S 数据输入 |
  | GAIN | 悬空 | — | 增益控制（悬空=9dB）|
  | SD | 悬空 | — | 关断控制（悬空=常开，接 GND=静音）|
  | GND | GND | — | 地线（必须共地）|
  | VIN | 3.3V/5V | — | 供电（5V 音量更大）|
  | OUT+ / OUT− | 喇叭两端 | — | 4~8Ω，无极性要求 |
  
  **增益配置**（GAIN 引脚）：
  | 接法 | 增益 |
  |------|------|
  | 悬空（默认） | 9 dB |
  | 接 GND | 6 dB |
  | 接 3V3 | 12 dB |
  | 100kΩ 接 GND | 3 dB |

### 4. 喇叭
- **无源喇叭**
  - 阻抗：4Ω
  - 功率：≤ 3W
  - 接线：直接焊在 MAX98357A 的 OUT+ / OUT− 端（无极性要求）

### 5. 10DOF 惯性测量单元
- **GY-91 MPU9250 + BMP280 模块（标准 8 针版本）**
  - 芯片组合：InvenSense MPU-9250 + Bosch BMP280
  - 10自由度：
    - 加速度计（3轴）：±2/±4/±8/±16g
    - 陀螺仪（3轴）：±250/±500/±1000/±2000°/s
    - 磁力计（3轴）：±4800μT
    - 气压计（1轴）：300-1100hPa
  - 分辨率：16位 ADC
  - 接口：I2C（默认）/ SPI
  - 供电：3.3V ~ 5V（板载 LDO）
  - 模块尺寸：14.3mm × 20.5mm
  - 引脚数：8 针（标准版本）
  
  **引脚定义**（I2C 模式）：
  | GY-91 | 接 ESP32-S3 | 说明 |
  |-------|-------------|------|
  | VIN | 5V（推荐）| 电源输入，经板载 LDO 稳压 |
  | GND | GND | 地线 |
  | 3V3 | NC（悬空）| 3.3V 输出（板载 LDO），可为其他设备供电 |
  | SCL | GPIO4 | I2C 时钟 |
  | SDA | GPIO3 | I2C 数据 |
  | SAO/SDO | GND | 地址选择引脚（复用）：GND=0x68/0x76，3.3V=0x69/0x77 |
  | NCS | NC（悬空）| MPU9250 SPI 片选（I2C 模式不接）|
  | CSB | NC（悬空）| BMP280 SPI 片选（I2C 模式不接）|
  
  **I2C 地址**（SAO/SDO = GND 时）：
  - MPU9250：0x68
  - BMP280：0x76
  
  **关键特性**：
  - SAO/SDO 是同一个引脚，同时控制两个芯片的地址
  - 内置 DMP（数字运动处理器），可直接输出姿态角
  - 磁力计需单独校准，受周围金属影响
  - 气压计可用于计算相对高度（精度约 ±1m）

### 6. MEMS 数字麦克风
- **INMP441 I2S 麦克风模块**
  - 型号：INMP441
  - 类型：MEMS 数字麦克风，I2S 接口
  - 采样率：支持 8kHz ~ 96kHz（典型 44.1kHz/48kHz）
  - 灵敏度：-26 dBFS
  - 信噪比：61 dB
  - 供电：3.3V（VDD）
  - 接口：I2S（数字音频）
  - 引脚数：6 针（两排，每排 3 针）
  
  **物理布局**（两排，共6针）：
  ```
  上排: SD    VDD    GND
  下排: L/R   WS     SCK
  ```

  **引脚定义**（与 MAX98357A 共用 I2S 总线，全双工模式）：
  | INMP441 | 接 ESP32-S3 | 说明 |
  |---------|-------------|------|
  | VDD | 3.3V | 供电 |
  | GND | GND | 地线（与系统共地）|
  | SCK | GPIO47 | I2S 位时钟（与 MAX98357A BCLK 共用）|
  | WS | GPIO21 | I2S 声道时钟（与 MAX98357A LRC 共用）|
  | SD | **GPIO2** | I2S 数据输出（麦克风 → ESP32）|
  | L/R | GND | 声道选择：GND=左声道，3.3V=右声道 |

  **关键特性**：
  - 与 MAX98357A 共用 BCLK/WS，ESP32-S3 I2S 全双工同时支持输入和输出
  - SD 是麦克风数据输出引脚，需单独占用一个 GPIO
  - L/R 引脚决定是左声道还是右声道数据
  - 无需外置 ADC，直接输出数字 I2S 信号

---

## GPIO 资源汇总

### 已分配 GPIO
| GPIO | 用途 | 模块 |
|------|------|------|
| GPIO1 | I2S DIN | MAX98357A（输出）|
| GPIO2 | I2S SD | INMP441（输入）|
| GPIO3 | I2C SDA | GY-91 |
| GPIO4 | I2C SCL | GY-91 |
| GPIO5 | BLK | ST7789 |
| GPIO6 | DC | ST7789 |
| GPIO7 | RES | ST7789 |
| GPIO9 | CS2 | ST7789 字库 |
| GPIO10 | CS1 | ST7789 屏幕 |
| GPIO11 | SDA/MOSI | ST7789 |
| GPIO12 | SCL/CLK | ST7789 |
| GPIO13 | FSO/MISO | ST7789 字库 |
| GPIO21 | I2S WS/LRC | MAX98357A + INMP441（共用）|
| GPIO47 | I2S BCLK | MAX98357A + INMP441（共用）|

### 空闲 GPIO（可扩展）
GPIO0, GPIO8, GPIO14~20, GPIO35~46, GPIO48

---

## 关键库依赖

Arduino IDE 需要安装以下库：

```
Adafruit ST7789          # 显示屏驱动
Adafruit GFX Library     # 图形库依赖
MPU9250_asukiaaa         # MPU9250 驱动
Adafruit BMP280 Library  # BMP280 气压计驱动
Adafruit Unified Sensor  # 传感器抽象层（依赖）
```

MAX98357A 使用 ESP32 内置 I2S 驱动，无需额外库。

---

## 新硬件记录规则

1. **当我提到新硬件时**，AI 应该先问："这是你第一次提到 **[硬件名称]**，请问你已经购买了这个硬件吗？"
2. **如果我回答已购买**：AI 应询问关键规格，并追加到上面的"已拥有硬件"列表中。
3. **如果我回答未购买**：AI 记录到"待购买清单"，并在我确认购买后移至"已拥有硬件"。

### 待购买清单
- （暂无）

---

## 记录历史

| 时间 | 操作 | 内容 |
|------|------|------|
| 2025-04-23 | 初始化 | 记录首批硬件：ESP32-S3-DevKitC-1、ST7789V 显示屏(10Pin带字库)、MAX98357A功放、喇叭 |
| 2025-04-23 | 补充细节 | 根据技术文档补充完整引脚定义、规格参数、库依赖 |
| 2025-04-23 | 新增硬件 | 添加 GY-91 10DOF 传感器（MPU9250+BMP280）|
| 2025-04-23 | 引脚校正 | 根据官方文档核实 ESP32-S3-DevKitC-1 v1.1 引脚布局，修正 GPIO3/4 位置（J1-13/J1-4）|
| 2025-04-23 | 新增硬件 | 添加 INMP441 I2S 数字麦克风（6针），与 MAX98357A 共用 I2S 总线，GPIO2 作为独立数据输入 |
| 2026-04-24 | 知识沉淀 | 创建 ST7789 显示屏工作原理详解文档，补充 ESP32 学习技巧（接线逻辑、编码思维、调试方法）|

---

*最后更新：2025-04-23（已校正官方引脚布局）*
