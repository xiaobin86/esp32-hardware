# ESP32-S3 + ST7789 + GT30L32S4W 汉字显示（硬件 SPI 版）

> 硬件：ESP32-S3-DevKitC-1-N16R8 + ZJY154S10Z0TG01（240×240 IPS 带字库版）  
> 开发环境：Arduino IDE  
> 本文是软件SPI版的进阶，重点讲清楚硬件SPI的工作原理、与字库共享总线的机制，以及为什么软件SPI版不能用 `SPI.begin()`、硬件SPI版却必须用。

---

## 一、硬件SPI vs 软件SPI

### 1.1 软件SPI（bit-bang）

软件SPI 完全由 CPU 执行，每个时钟脉冲都是一次 `digitalWrite`：

```
发送1字节 = 8次循环 × (拉低SCK + 设MOSI + 拉高SCK) = 24次 GPIO 操作
```

ESP32-S3 的 `digitalWrite` 每次约需 100ns，24次 = 2.4µs/字节，理论上限约 **400KB/s（3.2MHz 等效）**，实际更低。

### 1.2 硬件SPI（外设）

硬件SPI 由 ESP32-S3 内置的 SPI 控制器（FSPI）执行，CPU 只需将数据写入寄存器，剩下的由硬件自动完成。FSPI 支持 DMA，可在 CPU 完全不参与的情况下搬运大块数据。

本项目配置为 **40MHz**，理论峰值 **5MB/s**，是软件SPI的 **10~40 倍**。`fillScreen`（240×240×2=115200字节）从约 0.3 秒降至不到 30ms，肉眼可见地更流畅。

### 1.3 关键区别：引脚由谁控制

| 模式 | SCK / MOSI | 控制者 |
|------|-----------|--------|
| 软件SPI | `digitalWrite(GPIO12, HIGH/LOW)` | CPU，任何时候都能调 |
| 硬件SPI | SPI 外设通过 GPIO Matrix 路由 | SPI 控制器，CPU 写寄存器触发 |

这个区别是两个版本**最重要的约束来源**，后面会反复用到。

---

## 二、接线（与软件SPI版完全相同）

| 模块引脚 | 接 GPIO | J1 序号 | 说明 |
|----------|---------|---------|------|
| VCC | 3.3V | J1-1 | 供电 |
| GND | GND  | J1-22 | 地 |
| SCL | GPIO12 | J1-18 | SPI 时钟 |
| SDA | GPIO11 | J1-17 | SPI MOSI |
| RES | GPIO7  | J1-7  | 屏幕复位 |
| DC  | GPIO6  | J1-6  | 数据/命令 |
| CS1 | GPIO10 | J1-16 | 屏幕片选 |
| BLK | GPIO5  | J1-5  | 背光 |
| FSO | GPIO13 | J1-19 | 字库 MISO |
| CS2 | GPIO9  | J1-15 | 字库片选 |

硬件SPI版接线不变，变的只是代码。

---

## 三、为什么 ESP32-S3 的默认引脚刚好匹配

ESP32-S3 的 `pins_arduino.h`（`variants/esp32s3/`）定义了以下默认 SPI 引脚：

```cpp
static const uint8_t SS   = 10;   // = TFT_CS
static const uint8_t MOSI = 11;   // = TFT_MOSI
static const uint8_t MISO = 13;   // = TFT_MISO（字库回读）
static const uint8_t SCK  = 12;   // = TFT_SCLK
```

这四个默认值与本项目接线**完全一致**，因此：

- `SPI.begin()` 无参数调用，自动使用 GPIO10/11/12/13，**MISO 自动启用**
- Adafruit 内部调用 `SPI.begin()` 时同样使用这些默认值
- 不需要手动传引脚号，代码简洁

如果你使用的是其他 ESP32 型号（如 ESP32-WROOM），默认引脚不同，需要改为 `SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI)`。

---

## 四、Adafruit 构造函数决定了一切

```cpp
// 软件SPI：5个参数，传入 MOSI 和 SCLK 引脚号
Adafruit_ST7789 tft(CS, DC, MOSI, SCLK, RST);

// 硬件SPI：3个参数，不传引脚，由 SPI 对象管理
Adafruit_ST7789 tft(CS, DC, RST);
```

Adafruit_SPITFT 内部根据构造方式设置一个标志位，之后所有的 `tft.fillScreen()`、`tft.drawRGBBitmap()` 等操作都走对应的路径：

- 软件SPI 路径：调用 `digitalWrite(MOSI_pin, ...)` 和 `digitalWrite(SCLK_pin, ...)`
- 硬件SPI 路径：调用 `SPI.beginTransaction()` + `SPI.transfer()` + `SPI.endTransaction()`

**正是这个区别，决定了两个版本能不能调用 `SPI.begin()`**：

- 软件SPI：`SPI.begin()` 会把 GPIO11/12 接管给 SPI 外设，`digitalWrite` 失效 → **绝对不能调**
- 硬件SPI：本来就用 SPI 外设，`SPI.begin()` 是必须的初始化步骤 → **必须调**

---

## 五、硬件SPI下字库读取为何不冲突

### 5.1 `beginTransaction` 机制

硬件SPI 的每次操作被 `beginTransaction / endTransaction` 包裹：

```cpp
SPI.beginTransaction(SPISettings(速度, 位序, 模式));
// ... SPI.transfer() ...
SPI.endTransaction();
```

`beginTransaction` 做两件事：
1. 锁定 SPI 总线（防止中断或其他代码插入）
2. 将 SPI 外设配置为指定的速度、位序、时钟模式

`endTransaction` 释放总线锁，但**不改变引脚状态**，引脚保持最后一次传输后的电平。

### 5.2 屏幕和字库轮流使用同一总线

Adafruit 在每次屏幕操作时（`fillScreen`、`drawRGBBitmap` 等）内部调用：

```
startWrite():  CS1 拉低 → beginTransaction(屏幕参数)
  ... SPI.transfer(数据) ...
endWrite():    endTransaction() → CS1 拉高
```

我们在 `fontRead()` 中：

```
CS1 拉高（防御性）
beginTransaction(字库参数：8MHz, MODE0)
  FONT_CS 拉低
  SPI.transfer(0x03, 地址, 数据...)
  FONT_CS 拉高
endTransaction()
```

时序上是**严格串行**的：`drawHanzi()` 先调 `fontRead()`（读字库），再调 `tft.drawRGBBitmap()`（写屏幕），两段操作之间总线完全空闲，CS 都是高电平，绝对不会重叠。

### 5.3 为什么每次切换参数没有问题

字库：`SPISettings(8MHz, MSBFIRST, SPI_MODE0)`  
屏幕：`SPISettings(40MHz, MSBFIRST, SPI_MODE0)`（Adafruit 内部）

每次 `beginTransaction` 都重新写入 SPI 控制寄存器，完全覆盖上次的配置。SPI 外设本身没有"记忆"——寄存器写什么就是什么。所以频繁切换 8MHz ↔ 40MHz 是完全安全的，代价只是几十纳秒的寄存器写操作。

---

## 六、代码逐行解析

### 6.1 头文件与构造

```cpp
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>    // 硬件SPI版必须包含，软件SPI版不需要
```

硬件SPI版需要显式 `#include <SPI.h>`，因为代码中直接调用 `SPI.begin()` 和 `SPI.beginTransaction()`。

```cpp
#define TFT_SPI_FREQ   40000000UL   // 40MHz 写屏
#define FONT_SPI_FREQ   8000000UL   // 8MHz  读字库
```

两个频率常量分开定义，方便独立调整。字库芯片 GT30L32S4W 最高支持 80MHz，但与屏幕共享总线时保守用 8MHz，排查问题时还可以降到 1MHz。

```cpp
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
```

**3个参数**：Adafruit 识别为硬件SPI模式，内部存储 CS 和 DC 引脚号，SCK/MOSI 由 SPI 对象管理。

---

### 6.2 地址计算（与软件版相同）

```cpp
uint32_t gb2312Addr(uint8_t msb, uint8_t lsb) {
  if (msb >= 0xA1 && msb <= 0xA9 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xA1) * 94 + (lsb - 0xA1)) * 32 + 0x2C9D0;
  if (msb >= 0xB0 && msb <= 0xF7 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xB0) * 94 + (lsb - 0xA1) + 846) * 32 + 0x2C9D0;
  return 0;
}
```

地址计算逻辑不变，详见软件SPI版文档第三章。

---

### 6.3 字库读取（核心改动）

```cpp
bool fontRead(uint32_t addr) {
  digitalWrite(TFT_CS, HIGH);  // ①
```
① 防御性地确保屏幕 CS 为高。正常情况下 Adafruit 的 `endWrite()` 已经做了，但如果有异常中断 Adafruit 操作，这里做一次保险。

```cpp
  SPI.beginTransaction(SPISettings(FONT_SPI_FREQ, MSBFIRST, SPI_MODE0));  // ②
  digitalWrite(FONT_CS, LOW);  // ③
```
② `beginTransaction` 将 SPI 外设配置为 8MHz、MSB 优先、MODE0（CPOL=0 CPHA=0），同时加锁防止中断插入。  
③ CS2 拉低后字库芯片开始响应，SCK 此时为空闲低电平（MODE0）。

```cpp
  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >>  8) & 0xFF);
  SPI.transfer( addr        & 0xFF);
```
发送 READ 指令 `0x03` 和 24 位地址。`SPI.transfer(byte)` 同时发送和接收（全双工），发送阶段的返回值被丢弃，因为字库在收到地址前不会输出有效数据。

```cpp
  for (int i = 0; i < 32; i++) fontBuf[i] = SPI.transfer(0x00);
```
发送 32 个 dummy byte `0x00`，驱动 SCK 产生时钟，字库芯片在每个时钟周期将 MISO 数据推入 SPI 外设的接收寄存器，`SPI.transfer()` 返回接收到的字节。

```cpp
  digitalWrite(FONT_CS, HIGH);
  SPI.endTransaction();  // ④
```
④ CS2 先拉高结束字库通信，再 `endTransaction()` 解锁总线。顺序很重要：先释放 CS，再释放总线锁，确保字库芯片看到完整的 CS 高电平脉冲。

```cpp
  for (int i = 0; i < 32; i++) if (fontBuf[i]) return true;
  return false;
}
```
全零检测：正常汉字点阵不会是全零，全零意味着 MISO 始终为低（断线）或地址越界读到空白区。

---

### 6.4 绘制汉字（与软件版相同，速度更快）

```cpp
void drawHanzi(int16_t x, int16_t y, uint8_t msb, uint8_t lsb,
               uint16_t fg, uint16_t bg) {
  uint32_t addr = gb2312Addr(msb, lsb);
  if (!addr || !fontRead(addr)) return;

  for (int row = 0; row < 16; row++) {
    uint8_t b0 = fontBuf[row * 2];
    uint8_t b1 = fontBuf[row * 2 + 1];
    for (int col = 0; col < 8; col++) {
      pixBuf[row * 16 + col]     = (b0 & (0x80 >> col)) ? fg : bg;
      pixBuf[row * 16 + 8 + col] = (b1 & (0x80 >> col)) ? fg : bg;
    }
  }
  tft.drawRGBBitmap(x, y, pixBuf, 16, 16);
}
```

逻辑与软件版完全相同：读32字节点阵 → 解码为 RGB565 像素缓冲 → 一次性写入屏幕。

区别在于最后的 `tft.drawRGBBitmap()`：软件版通过 `digitalWrite` bit-bang 传输 512 字节像素数据；硬件版通过 SPI 外设的 DMA 传输，速度快得多。绘制一整屏汉字在硬件版中几乎感觉不到延迟。

---

### 6.5 setup() 初始化

```cpp
SPI.begin();
```
无参数调用 `SPI.begin()`，使用 ESP32-S3 FSPI 默认引脚（SCK=12, MOSI=11, MISO=13, SS=10），与本项目接线完全一致。这一行做了三件事：
1. 配置 GPIO12 为 SPI 外设时钟输出
2. 配置 GPIO11 为 SPI 外设数据输出（MOSI）
3. 配置 GPIO13 为 SPI 外设数据输入（MISO）—— **这是关键，字库读取需要 MISO**

如果跳过 `SPI.begin()` 直接 `tft.init()`，Adafruit 内部也会调用 `SPI.begin()`，但 Adafruit 不知道 MISO 的存在（屏幕是只写设备，不需要 MISO），可能以不包含 MISO 的方式初始化 SPI。为了确保 MISO 被正确配置，**显式提前调用 `SPI.begin()`**。

```cpp
tft.init(240, 240, SPI_MODE0);
```
传入 `SPI_MODE0` 与字库芯片的时序一致（都是 CPOL=0 CPHA=0），保持整个总线的模式统一，减少切换时的潜在干扰。

```cpp
tft.invertDisplay(true);
```
IPS 屏幕必须调用，发送 `INVON (0x21)` 命令翻转像素极性。不调用时颜色全部反相，白色显示为黑色，内容不可见。

---

## 七、两个版本的设计对比

| 项目 | 软件SPI版 | 硬件SPI版 |
|------|---------|---------|
| Adafruit 构造 | `(CS, DC, MOSI, SCK, RST)` 5参 | `(CS, DC, RST)` 3参 |
| 屏幕刷新速度 | ~1MHz，bit-bang | ~40MHz，SPI外设+DMA |
| `SPI.begin()` | **不能调用**，否则破坏 bit-bang | **必须调用**，配置 MISO |
| 字库读取 | 软件 bit-bang | `SPI.beginTransaction()` |
| 字库读取速度 | ~1MHz | 8MHz |
| `fillScreen` 耗时 | ~300ms | ~23ms |
| 绘制10个汉字 | ~200ms | ~30ms |
| 代码复杂度 | 需要 `fontSend/fontRecv` | 只需 `SPI.transfer` |
| 移植性 | 任何 GPIO 均可 bit-bang | 依赖 SPI 外设引脚 |

---

## 八、常见问题

### 为什么 `SPI.begin()` 必须在 `tft.init()` 之前？

`tft.init()` 内部会调用 `SPI.begin()`，但调用方式不传 MISO 引脚（屏幕不需要 MISO）。如果 Adafruit 的 `SPI.begin()` 覆盖了 MISO 的配置，后续字库读取将收到全零。

解决方法：在 `tft.init()` 之前显式调用 `SPI.begin()`，提前将包括 MISO 在内的四个引脚都配置好。Adafruit 后续再调用 `SPI.begin()` 时，由于引脚已配置且 SPI 已初始化，行为上会跳过重复配置。

### 字库读取失败（全零）的排查顺序

1. 用万用表确认 GPIO9（CS2）和 GPIO13（MISO/FSO）有信号
2. 用示波器或逻辑分析仪确认 CS2 拉低后 SCK 有时钟脉冲，FSO 有数据
3. 检查 CS1（GPIO10）在读字库时是否为高电平（如果 CS1 也为低，屏幕会干扰 MISO）
4. 降低字库 SPI 频率：将 `FONT_SPI_FREQ` 从 8MHz 改为 1MHz 测试

### 能不能把屏幕也降到 8MHz？

可以，修改 `tft.init()` 之前的 SPI 初始化频率，或在 `Adafruit_ST7789` 的 `init()` 参数中传入频率。但屏幕支持最高 62.5MHz（ST7789V datasheet），40MHz 已是合理值，降频只会让刷新变慢。

---

## 九、GB2312 汉字速查（常用字 \x 转义）

| 汉字 | `\x` 转义 | 汉字 | `\x` 转义 |
|------|----------|------|----------|
| 中 | `\xD6\xD0` | 国 | `\xB9\xFA` |
| 你 | `\xC4\xE3` | 好 | `\xBA\xC3` |
| 硬 | `\xD3\xB2` | 件 | `\xBE\xFE` |
| 版 | `\xB0\xE6` | 本 | `\xB1\xBE` |
| 温 | `\xCE\xC2` | 度 | `\xB6\xC8` |
| 湿 | `\xCA\xAD` | 红 | `\xBA\xEC` |
| 色 | `\xC9\xAB` | 绿 | `\xC2\xCC` |
| 蓝 | `\xC0\xB6` | 黄 | `\xBB\xC6` |
| 第 | `\xB5\xDA` | 行 | `\xD0\xD0` |
| 一 | `\xD2\xBB` | 二 | `\xB6\xFE` |
| 三 | `\xC8\xFD` | 四 | `\xCB\xC4` |

计算任意汉字的 GB2312 编码：

```python
"你想要的汉字".encode('gb2312').hex()
# 输出示例：'d6d0' → \xD6\xD0
```

---

## 十、所需 Arduino 库

| 库名 | 用途 |
|------|------|
| Adafruit ST7789 | ST7789 驱动 |
| Adafruit GFX Library | 图形基础库 |
| Adafruit BusIO | 总线抽象层（依赖） |

板子配置（工具菜单）：

| 选项 | 值 |
|------|---|
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB |
| PSRAM | OPI PSRAM |
| USB Mode | Hardware CDC and JTAG |
| Upload Speed | 921600 |
