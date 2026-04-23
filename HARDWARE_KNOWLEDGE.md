# 硬件知识库

> 基于我的实际硬件（ESP32-S3-DevKitC-1 + ST7789V 10Pin 带字库屏 + MAX98357A + 喇叭 + GY-91 + INMP441）的接线知识、代码示例和常见问题。

## 网络配置

> WiFi 凭据已移至 `config.h`（该文件被 `.gitignore` 保护，不会提交到 GitHub）。
> 首次使用时请复制 `config.h` 中的配置或创建自己的版本。

---

## ST7789V 带字库显示屏

### 物理布局（上下两排，共10针）

```
上排: FSO    CS1    RES    SCL    GND
下排: CS2    BLK    DC     SDA    VCC
```

### 接线要点
- 全部 10 根线接在 **J1 左侧**，与 MAX98357A 的 J3 右侧完全隔离
- SDA/SCL 是屏和字库 **共用** 的，靠 CS1（屏幕片选）和 CS2（字库片选）分时控制
- 任意时刻只能拉低一个 CS，另一个必须保持高电平
- FSO（MISO）只有字库芯片会回读数据，屏幕是只写设备

### 关键代码片段

#### 1. 基础屏幕初始化（验证接线）
```cpp
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL     5

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);   // 背光开
  
  tft.init(240, 240);            // 分辨率 240×240
  tft.setRotation(0);            // 0/1/2/3 调方向
  tft.fillScreen(ST77XX_BLACK);
}
```

#### 2. 字库芯片读取（GT30L32S4W）
```cpp
#define FONT_CS 9   // 字库片选

// 读取字库数据
void fontChipRead(uint32_t addr, uint8_t* buf, uint16_t len) {
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(FONT_CS, LOW);
  
  SPI.transfer(0x03);                      // 读命令
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
  uint8_t  qu  = h - 0xA0;
  uint8_t  wei = l - 0xA0;
  return 0x2C9D0 + ((uint32_t)(qu - 1) * 94 + (wei - 1)) * 32;
}
```

### 常见问题速查

| 现象 | 原因 | 解决方法 |
|------|------|----------|
| 全白屏，背光亮 | BLK 未接高电平 | BLK 接 3V3 或代码 `digitalWrite(5, HIGH)` |
| 全白屏，背光暗 | CS1/CS2 接反 | 将 IO9 和 IO10 互换，代码同步修改 |
| 花屏 / 颜色错 | RGB 字节序问题 | 初始化后加 `tft.invertDisplay(true)` |
| 方向不对 | Rotation 设置 | 改 `tft.setRotation(1/2/3)` |
| 串口显示字库失败 | CS2 或 FSO 接错 | 检查 IO9（CS2）和 IO13（FSO）接线 |
| 汉字显示乱码 | 文件编码不是 GB2312 | VS Code 重新以 GB2312 编码保存 |

---

## MAX98357A I2S 功放

### 物理布局（从左到右，共7针）
```
LRC    BCLK    DIN    GAIN    SD    GND    VIN
```

### 接线要点
- 接在 **J3 右侧**，与 ST7789 的 J1 左侧完全物理隔离
- **必须共地**：MAX98357A 的 GND 必须接 ESP32 的 GND
- VIN 接 3.3V 或 5V，5V 音量更大
- SD 悬空 = 常开；接 GND = 静音

### 关键代码片段

#### I2S 初始化
```cpp
#include "driver/i2s.h"

#define I2S_BCLK  47
#define I2S_LRC   21
#define I2S_DIN    1
#define I2S_PORT  I2S_NUM_0
#define SAMPLE_RATE 44100

void i2s_init() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 128,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);

  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_LRC,
    .data_out_num = I2S_DIN,
    .data_in_num  = I2S_PIN_NO_CHANGE,
  };
  i2s_set_pin(I2S_PORT, &pins);
}
```

### 常见问题速查

| 现象 | 原因 | 解决方法 |
|------|------|----------|
| 完全无声 | BCLK/LRC/DIN 某根接错 | 逐根对照接线表检查 |
| 嗡嗡电流声 | GND 未共地 | 确认 MAX98357A GND 接 ESP32 GND |
| 声音很小 | GAIN 不够 | GAIN 接 3V3，提升到 12dB |
| 爆音/杂音 | DMA 缓冲太小 | `dma_buf_count=16, dma_buf_len=128` |
| 编译报错找不到 i2s.h | Arduino Core 版本过旧 | 升级到 ESP32 Arduino Core 2.x 或 3.x |

---

## INMP441 I2S 数字麦克风

### 物理布局（两排，共6针）
```
上排: SD    VDD    GND
下排: L/R   WS     SCK
```

### 接线要点
- 与 MAX98357A **共用 I2S 总线**：SCK→GPIO47（BCLK）、WS→GPIO21（LRC）
- SD 单独接 GPIO2，ESP32-S3 I2S 全双工同时支持输入（INMP441）和输出（MAX98357A）
- L/R 接 GND = 左声道，接 3.3V = 右声道
- 必须共地

> **为什么共用 BCLK/WS 不会冲突？**
> 
> I2S 是**总线协议**，BCLK（位时钟）和 WS（声道时钟）是**广播信号**，所有设备同步在同一时钟上：
> - **时钟线（BCLK/WS）**：由 ESP32 主控产生，同时连接到 MAX98357A 和 INMP441，只传输时序，不传输数据
> - **数据线（点对点）**：
>   - `GPIO1 → MAX98357A DIN`：ESP32 **输出**音频数据给功放
>   - `GPIO2 ← INMP441 SD`：ESP32 **输入**音频数据从麦克风
> 
> ESP32 配置为 `I2S_MODE_TX | I2S_MODE_RX` 全双工模式后，硬件自动在发送时驱动 GPIO1，在采样时读取 GPIO2，两者通过同一组 BCLK/WS 保持时序同步。就像 I2C 的 SCL 是所有设备共享的时钟线，SDA 根据传输方向决定谁是发送方一样。

### 引脚定义
| INMP441 | 接 ESP32-S3 | J3 位置 | 功能说明 |
|---------|-------------|---------|----------|
| VDD | 3.3V | — | 供电 |
| GND | GND | — | 地线 |
| SCK | GPIO47 | J3-17 | I2S 位时钟（与 MAX98357A 共用）|
| WS | GPIO21 | J3-18 | I2S 声道时钟（与 MAX98357A 共用）|
| SD | GPIO2 | J3-5 | I2S 数据输出（麦克风 → ESP32）|
| L/R | GND | — | 左声道选择 |

### 关键代码片段

#### I2S 全双工初始化（同时支持麦克风和功放）
```cpp
#include "driver/i2s.h"

#define I2S_BCLK   47
#define I2S_WS     21
#define I2S_DOUT    1   // MAX98357A
#define I2S_DIN     2   // INMP441
#define I2S_PORT   I2S_NUM_0
#define SAMPLE_RATE 44100

void i2s_init_duplex() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 128,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);

  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_DIN,
  };
  i2s_set_pin(I2S_PORT, &pins);
}
```

### 常见问题速查

| 现象 | 原因 | 解决方法 |
|------|------|----------|
| 录音全是 0 | SD 引脚接错或 L/R 配置错误 | 检查 GPIO2 接线，确认 L/R=GND |
| 录音有杂音 | 电源不稳 | VDD 并接 100nF 陶瓷电容到 GND |
| 与功放冲突 | I2S 模式未设全双工 | 确认 mode 包含 `I2S_MODE_TX \| I2S_MODE_RX` |
| 左右声道反了 | L/R 接反 | L/R 接 GND=左声道，接 3.3V=右声道 |

---

## 多模块同时使用

所有模块 **可以同时使用**，GPIO 区域隔离或总线共享：

| 模块 | 使用排针 | GPIO |
|------|---------|------|
| ST7789 显示屏 | J1 左侧 | GPIO5/6/7/9/10/11/12/13 |
| GY-91 传感器 | J1 左侧 | GPIO3/4 |
| MAX98357A 功放 | J3 右侧 | GPIO1/21/47 |
| INMP441 麦克风 | J3 右侧 | GPIO2/21/47 |

**I2S 总线共享**：
- MAX98357A（输出）和 INMP441（输入）共用 BCLK(GPIO47) + WS(GPIO21)
- 数据线独立：GPIO1（输出到功放）、GPIO2（输入从麦克风）
- ESP32-S3 I2S 全双工模式同时支持 TX + RX

所有代码可以直接合并到同一个 Arduino 项目中。

---

## ESP32 学习技巧

> 详细内容见独立文档：`ESP32学习技巧-接线逻辑与编码思维.md`  
> 涵盖：三层排查法、先配置后使用、总线点名制、延迟刷新、完整思维链条、分层学习路径、调试工具推荐、常见误区。

---

### ESP32-S3-DevKitC-1 v1.1 官方引脚布局（已验证）

**参考文档**：[ESP32-S3-DevKitC-1 v1.1 用户指南 - 排针](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html)

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

**已验证接线**：
- ST7789：GPIO5/6/7/9/10/11/12/13（J1 左侧）
- MAX98357A：GPIO1/21/47（J3 右侧）
- GY-91：GPIO3（J1-13）/ GPIO4（J1-4）
- INMP441：GPIO2/21/47（J3 右侧，与 MAX98357A 共用 BCLK/WS）

---

*基于 `esp32-hardware` 技术文档整理*
*官方引脚已验证：2025-04-23*
