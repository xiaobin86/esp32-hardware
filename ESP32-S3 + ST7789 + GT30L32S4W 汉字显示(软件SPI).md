

> 硬件：ESP32-S3-DevKitC-1-N16R8 + ZJY154S10Z0TG01（240×240 IPS 带字库版）  
> 开发环境：Arduino IDE  
> 最终效果：从字库芯片读取 GB2312 点阵，在 ST7789 彩屏上显示汉字

---

## 一、硬件概述

### 1.1 显示模块引脚（10 针，双排）

模块物理布局（面朝针脚，上下两排）：

```
上排（从左到右）：FSO   CS1   RES   SCL   GND
下排（从左到右）：CS2   BLK   DC    SDA   VCC
```

官方规格书定义（ZJY154S10Z0TG01 Pin Descriptions）：

| 引脚号 | 名称 | 方向 | 说明 |
|--------|------|------|------|
| 1 | GND | — | 地 |
| 2 | VCC | 输入 | 3.0 ~ 3.3V 供电 |
| 3 | SCL | 输入 | SPI 时钟（屏幕+字库共用） |
| 4 | SDA | 输入 | SPI 数据输入（屏幕+字库共用） |
| 5 | RES | 输入 | 复位，低电平有效 |
| 6 | DC  | 输入 | 数据/命令选择（高=数据，低=命令） |
| 7 | CS1 | 输入 | 屏幕片选，低有效 |
| 8 | BLK | 输入 | 背光控制，高=开 |
| 9 | FSO | 输出 | 字库芯片数据输出（MISO） |
| 10 | CS2 | 输入 | 字库芯片片选，低有效 |

**关键设计**：SCL/SDA 由屏幕和字库**共用**，通过 CS1/CS2 分时片选——任意时刻只拉低一个 CS，另一个必须保持高电平。

### 1.2 接线表（接 ESP32-S3-DevKitC-1 J1 左侧）

| 模块引脚 | 接 GPIO | J1 序号 | 说明 |
|----------|---------|---------|------|
| VCC | 3.3V | J1-1 | 供电 |
| GND | GND | J1-22 | 地 |
| SCL | GPIO12 | J1-18 | SPI 时钟（屏幕+字库） |
| SDA | GPIO11 | J1-17 | SPI MOSI（屏幕+字库） |
| RES | GPIO7  | J1-7  | 屏幕复位 |
| DC  | GPIO6  | J1-6  | 数据/命令 |
| CS1 | GPIO10 | J1-16 | 屏幕片选 |
| BLK | GPIO5  | J1-5  | 背光 |
| FSO | GPIO13 | J1-19 | 字库 MISO（回读） |
| CS2 | GPIO9  | J1-15 | 字库片选 |

---

## 二、核心芯片原理

### 2.1 ST7789V 显示驱动

ST7789V 是一款 240×320 TFT 控制器（本模块只用 240×240 区域），4 线 SPI 接口：
- **CS（片选）**：低电平时 MCU 可以与芯片通信
- **DC（数据/命令）**：低电平发送寄存器命令，高电平发送显示数据
- **SDA（MOSI）**：数据单向写入，屏幕是只写设备，不回读
- **SCL（CLK）**：SPI 时钟

**IPS 屏的关键差异**：ZJY154S10Z0TG01 使用 IPS 面板，出厂默认的像素极性与普通 TFT 相反。必须发送 `INVON（0x21）` 命令（或调用 `invertDisplay(true)`），否则所有颜色显示为反相，白色背景看起来是黑色，内容完全不可见。

写入显示内容的流程：
```
1. CS1 拉低（选中屏幕）
2. DC 拉低，发送命令 0x2A（设置列范围）
3. DC 拉高，发送 4 字节列起止坐标
4. DC 拉低，发送命令 0x2B（设置行范围）
5. DC 拉高，发送 4 字节行起止坐标
6. DC 拉低，发送命令 0x2C（开始写 GRAM）
7. DC 拉高，连续发送像素数据（每像素 2 字节 RGB565）
8. CS1 拉高（释放屏幕）
```

### 2.2 GT30L32S4W 字库芯片

GT30L32S4W 是一颗 SPI 接口的只读字库 ROM，内置 GB2312 全集（6763 汉字 + 846 符号），支持 12×12、16×16、24×24、32×32 四种点阵尺寸。本项目使用 **15×16 点阵**（官方称"15×16"，存储为每行 2 字节 × 16 行 = 32 字节，实际有效宽度 15 像素，第 16 位补 0）。

芯片只有两条操作指令：

| 指令 | 字节码 | 说明 |
|------|--------|------|
| READ | `0x03` | 普通读取，3 字节地址 + 连续读数据 |
| FAST_READ | `0x0B` | 快速读取，地址后需额外 1 个 Dummy Byte |

读取时序（来自 datasheet 第 2 章）：
```
CS2 拉低
→ 发送 0x03（READ 指令）
→ 发送地址高字节（addr[23:16]）
→ 发送地址中字节（addr[15:8]）
→ 发送地址低字节（addr[7:0]）
→ 连续读取 N 字节（SCK 上升沿锁存写入，下降沿输出读出）
CS2 拉高
```

---

## 三、GB2312 字库寻址公式

来源：GT30L32S4W 官方 datasheet 第 4.1.2 节（15×16 点阵）。

```
基地址 BaseAdd = 0x2C9D0

符号区（Zone 1，编码 0xA1A1 ~ 0xA9EF）：
  Address = ((MSB - 0xA1) × 94 + (LSB - 0xA1)) × 32 + BaseAdd

汉字区（Zone 2，编码 0xB0A1 ~ 0xF7FE）：
  Address = ((MSB - 0xB0) × 94 + (LSB - 0xA1) + 846) × 32 + BaseAdd
```

参数说明：
- **MSB**：GB2312 编码高字节（区码 + 0xA0）
- **LSB**：GB2312 编码低字节（位码 + 0xA0）
- **×32**：每个字符占 32 字节（2字节/行 × 16行）
- **+846**：Zone 2 汉字前面有 9 个符号区 × 94 个字符 = 846 个符号，汉字从第 847 个槽位开始

**验证示例——"中"字（GB2312: 0xD6 0xD0）**：
```
MSB = 0xD6, LSB = 0xD0
属于 Zone 2（0xD6 >= 0xB0）
Address = ((0xD6 - 0xB0) × 94 + (0xD0 - 0xA1) + 846) × 32 + 0x2C9D0
        = (38 × 94 + 47 + 846) × 32 + 0x2C9D0
        = 4465 × 32 + 0x2C9D0
        = 0x4F7F0
```

---

## 四、SPI 总线冲突的根本原因与解法

### 4.1 问题复现

调试过程中遇到一个规律性现象：**加入字库读取后屏幕变空白**，但串口显示字库数据读取成功。

### 4.2 根本原因

Adafruit_ST7789 有两种构造方式，参数个数决定了 SPI 模式：

```cpp
// 3 个参数 → 硬件 SPI（使用 SPI 外设，速度快）
Adafruit_ST7789 tft(CS, DC, RST);

// 5 个参数 → 软件 SPI（bit-bang，用 digitalWrite 模拟时序）
Adafruit_ST7789 tft(CS, DC, MOSI, SCLK, RST);
```

本项目传了 5 个参数，因此 Adafruit **使用软件 SPI**，底层是 `digitalWrite(GPIO12, HIGH/LOW)` 来驱动时钟线。

然而之前的代码在 `fontRead()` 里调用了 `SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI)`：

```cpp
// 错误做法
void fontRead(...) {
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);  // ← 致命错误
    ...
}
```

`SPI.begin()` 会将 GPIO11、GPIO12、GPIO13 切换为**硬件 SPI 外设功能**（通过 ESP32 的 GPIO Matrix 路由）。切换之后，`digitalWrite(GPIO12, ...)` 不再控制实际引脚输出——GPIO 已被 SPI 外设接管。Adafruit 的软件 SPI 从此失效，屏幕一片空白。

### 4.3 解决方案

字库读取**也用软件 SPI**（bit-bang），完全不调用 `SPI.begin()`，两者都通过 `digitalWrite` 操作 GPIO，互不干扰。

```
屏幕写入：Adafruit bit-bang → CS1低，操作GPIO11/12 → CS1高
字库读取：我们的bit-bang  → CS2低，操作GPIO11/12/13 → CS2高
```

两者共用 GPIO11（MOSI）和 GPIO12（SCK），通过 CS1/CS2 分时互斥，天然隔离。

---

## 五、代码逐行解析

### 5.1 头文件与引脚定义

```cpp
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
```
- `Adafruit_GFX`：通用图形库，提供 `drawPixel`、`fillRect`、`print` 等接口
- `Adafruit_ST7789`：ST7789 驱动，继承自 GFX，负责发送初始化序列和控制命令

```cpp
#define TFT_CS    10   // 屏幕片选  CS1
#define TFT_DC     6   // 数据/命令
#define TFT_RST    7   // 复位
#define TFT_MOSI  11   // SPI MOSI（屏幕+字库共用）
#define TFT_SCLK  12   // SPI CLK （屏幕+字库共用）
#define TFT_MISO  13   // SPI MISO（字库回读，屏幕不用）
#define TFT_BL     5   // 背光
#define FONT_CS    9   // 字库片选 CS2
```
8 个引脚的分工：前 5 个（CS/DC/RST/MOSI/SCLK）是屏幕通信必需；MISO 只有字库读取时用到，屏幕写数据不需要 MISO；BLK 控制背光；FONT_CS 是字库独有的片选。

```cpp
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
```
5 个参数构造函数 → 软件 SPI 模式。Adafruit 内部将这 5 个 GPIO 号存起来，驱动时用 `digitalWrite` 逐位翻转，不使用 ESP32 的 SPI 硬件外设。

```cpp
static uint8_t  fontBuf[32];
static uint16_t pixBuf[16 * 16];
```
- `fontBuf`：存储从字库芯片读取的 32 字节原始点阵数据（2字节/行 × 16行）
- `pixBuf`：存储转换后的 16×16 个 RGB565 像素值，供 `drawRGBBitmap` 一次性写屏

两者声明为 `static`，防止在栈上分配（ESP32 栈空间较小，256 字节 + 512 字节放栈上容易溢出）。

---

### 5.2 GB2312 地址计算

```cpp
uint32_t gb2312Addr(uint8_t msb, uint8_t lsb) {
  if (msb >= 0xA1 && msb <= 0xA9 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xA1) * 94 + (lsb - 0xA1)) * 32 + 0x2C9D0;
  if (msb >= 0xB0 && msb <= 0xF7 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xB0) * 94 + (lsb - 0xA1) + 846) * 32 + 0x2C9D0;
  return 0;
}
```

**第一个 if（Zone 1 符号区）**：  
- 判断条件：高字节 0xA1~0xA9，即 GB2312 第 1~9 区（标点、数字、字母等符号）
- `msb - 0xA1`：把区码转为 0 起的行号（第 1 区 → 0，第 9 区 → 8）
- 每区 94 个字符：`行号 × 94 + 列号 = 该字符在符号区内的偏移量`
- `× 32`：每字符 32 字节
- `+ 0x2C9D0`：15×16 点阵在 ROM 中的起始地址

**第二个 if（Zone 2 汉字区）**：  
- 判断条件：高字节 0xB0~0xF7，即 GB2312 第 16~87 区（6763 个汉字）
- `+ 846`：符号区共 9 行 × 94 个 = 846 个槽位，汉字从第 847 个开始，所以偏移量要加 846
- 注意：GB2312 第 10~15 区（0xAA~0xAF）是空区，字库芯片直接跳过不分配空间，这就是为什么汉字区公式要用 `msb - 0xB0` 而不是 `msb - 0xAA`

**返回 0**：编码不在有效范围内，调用方需检查返回值。

---

### 5.3 软件 SPI 发送

```cpp
void fontSend(uint8_t dat) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(TFT_SCLK, LOW);             // SCK 拉低（准备阶段）
    digitalWrite(TFT_MOSI, (dat & 0x80) ? HIGH : LOW);  // 输出最高位
    dat <<= 1;                               // 数据左移，下一位准备好
    digitalWrite(TFT_SCLK, HIGH);            // SCK 拉高（上升沿，字库锁存数据）
  }
  digitalWrite(TFT_SCLK, LOW);              // 发完后时钟回到空闲低电平
}
```

每次循环处理 1 个 bit，**MSB first（高位先发）**，时序遵循 SPI MODE0：
- 时钟空闲时为低电平（CPOL=0）
- 数据在上升沿被对端锁存（CPHA=0）

`dat & 0x80` 取最高位，然后 `dat <<= 1` 把次高位移到最高位，下次循环发送次高位，以此类推。

---

### 5.4 软件 SPI 接收

```cpp
uint8_t fontRecv() {
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(TFT_SCLK, HIGH);                        // SCK 上升沿
    digitalWrite(TFT_SCLK, LOW);                         // SCK 下降沿→字库输出数据
    data = (data << 1) | digitalRead(TFT_MISO);          // 采样 MISO
  }
  return data;
}
```

GT30L32S4W 的读时序（来自 datasheet + 厂商 C51 示例代码对照）：字库在 **SCK 下降沿**将下一位数据推到 MISO 线上，主机在 SCK 为低期间采样是最安全的。

逐步分解：
1. `SCK HIGH`→`SCK LOW`：产生一个完整的时钟脉冲，下降沿触发字库推出 1 bit
2. `data << 1`：为新 bit 腾出最低位
3. `| digitalRead(TFT_MISO)`：读 MISO 当前电平，与入 data 最低位

---

### 5.5 从字库读取 32 字节

```cpp
bool fontRead(uint32_t addr) {
  digitalWrite(TFT_CS,  HIGH);    // 确保屏幕 CS1 为高（屏幕不参与）
  digitalWrite(TFT_SCLK, LOW);   // 时钟线归位空闲低电平

  digitalWrite(FONT_CS, LOW);    // 拉低 CS2，选中字库芯片
  delayMicroseconds(2);          // 给字库芯片反应时间

  fontSend(0x03);                        // READ 指令
  fontSend((addr >> 16) & 0xFF);         // 地址高字节 [23:16]
  fontSend((addr >>  8) & 0xFF);         // 地址中字节 [15:8]
  fontSend( addr        & 0xFF);         // 地址低字节 [7:0]

  for (int i = 0; i < 32; i++) fontBuf[i] = fontRecv();  // 连续读 32 字节

  digitalWrite(FONT_CS, HIGH);   // CS2 拉高，结束本次操作
  delayMicroseconds(2);

  for (int i = 0; i < 32; i++) if (fontBuf[i]) return true;
  return false;                  // 全零 = 地址错误或接线故障
}
```

**地址拆分**：`addr` 是 24 位地址（字库芯片最大 16MB），用位移和掩码拆成 3 个字节依次发送，高字节先发。

**全零检测**：正常的汉字点阵不会是全零（没有空白字符），如果 32 字节全是 0x00，说明读取失败——通常是 CS2 或 MISO 接线问题。

---

### 5.6 绘制单个汉字

```cpp
void drawHanzi(int16_t x, int16_t y, uint8_t msb, uint8_t lsb,
               uint16_t fg, uint16_t bg) {
  uint32_t addr = gb2312Addr(msb, lsb);
  if (!addr || !fontRead(addr)) return;    // 地址无效或读取失败则跳过

  for (int row = 0; row < 16; row++) {
    uint8_t b0 = fontBuf[row * 2];         // 该行左半部分（第 0~7 列）
    uint8_t b1 = fontBuf[row * 2 + 1];    // 该行右半部分（第 8~15 列）
    for (int col = 0; col < 8; col++) {
      pixBuf[row * 16 + col]     = (b0 & (0x80 >> col)) ? fg : bg;
      pixBuf[row * 16 + 8 + col] = (b1 & (0x80 >> col)) ? fg : bg;
    }
  }
  tft.drawRGBBitmap(x, y, pixBuf, 16, 16);
}
```

**点阵解析**：每行 2 字节共 16 位，每一位代表 1 个像素，`1` = 前景色，`0` = 背景色。

`b0 & (0x80 >> col)`：用移位的掩码依次检测第 0、1、2...7 位是否为 1。`0x80 >> 0 = 0b10000000`（检测最高位），`0x80 >> 1 = 0b01000000`（检测次高位），以此类推，MSB 对应最左列。

**`drawRGBBitmap` 的优势**：将 256 个像素一次性以矩形窗口写入屏幕，内部只调用一次 `setAddrWindow` + 一次连续 DMA/SPI 传输，比 256 次 `drawPixel` 快约 **20 倍**。

---

### 5.7 显示字符串

```cpp
void drawStr(int16_t x, int16_t y, const char* str, uint16_t fg, uint16_t bg) {
  int16_t cx = x, cy = y;      // 当前光标位置
  while (*str) {
    uint8_t c = (uint8_t)*str;
    if (c == '\n') { cx = x; cy += 18; str++; continue; }  // 换行：回到左边距，下移 18px

    if (c >= 0x80) {            // 双字节汉字：高字节 >= 0x80
      uint8_t lsb = (uint8_t)*(str + 1);
      if (!lsb) break;          // 防止字符串结尾只有高字节的异常
      if (cx + 16 > 240) { cx = x; cy += 18; }  // 超出右边界则换行
      if (cy + 16 > 240) break;                  // 超出底部则停止
      drawHanzi(cx, cy, c, lsb, fg, bg);
      cx += 16; str += 2;       // 汉字宽 16px，指针跳 2 字节
    } else {
      if (cx + 6 > 240) { cx = x; cy += 18; }
      tft.setTextSize(1);
      tft.setTextColor(fg, bg);
      tft.setCursor(cx, cy + 4);    // +4px 使 ASCII 与汉字底部视觉对齐
      tft.print((char)c);
      cx += 6; str++;           // ASCII 字符宽约 6px，指针跳 1 字节
    }
  }
}
```

**汉字判断依据**：GB2312 所有双字节字符的高字节都 >= 0x80，ASCII 字符都 < 0x80，因此 `c >= 0x80` 可以精确区分。

**字符串编码**：代码中汉字用 `\xHH\xHH` 十六进制转义表示 GB2312 编码，例如"中国"写作 `"\xD6\xD0\xB9\xFA"`。这种写法与源文件保存格式（UTF-8 或 GB2312）无关，字节值始终正确。

---

### 5.8 setup() 初始化流程

```cpp
Serial.begin(115200);
uint32_t t0 = millis();
while (!Serial && millis() - t0 < 5000) delay(10);
```
ESP32-S3 使用 USB CDC 虚拟串口时，串口枚举需要时间。最多等 5 秒，超时自动继续（防止没开串口监视器时程序卡死）。

```cpp
pinMode(TFT_BL,   OUTPUT); digitalWrite(TFT_BL,   HIGH);  // 背光立即开启
pinMode(FONT_CS,  OUTPUT); digitalWrite(FONT_CS,  HIGH);  // 字库 CS 默认高（未选中）
pinMode(TFT_MISO, INPUT);                                  // MISO 只读，不配置为输出
```
FONT_CS 在初始化时必须拉高，否则字库芯片在 CS 悬空期间可能输出随机数据，干扰 MISO 线。

```cpp
tft.init(240, 240);
tft.setRotation(0);
tft.invertDisplay(true);   // IPS 屏必须！
```
`tft.init(240, 240)` 发送完整的 ST7789 初始化序列（SLPOUT → 120ms 延迟 → 各寄存器配置 → DISPON），最后屏幕进入正常显示模式。

`invertDisplay(true)` 发送 `INVON（0x21）`命令，翻转像素极性。不加这行，IPS 屏上所有颜色显示为补色，白色背景呈现为黑色，内容完全不可见。

```cpp
tft.fillScreen(0xF800);    // RGB565 纯红色
delay(2000);
```
第一件事画红色全屏而不是黑色：黑色填充无法区分"屏幕正常显示黑色"和"屏幕没工作"，红色可以立即判断屏幕是否点亮。

---

## 六、RGB565 颜色格式

ST7789 使用 16 位 RGB565 格式，每像素 2 字节：

```
bit 15~11：红色（5位，0~31）
bit 10~5 ：绿色（6位，0~63）
bit 4~0  ：蓝色（5位，0~31）
```

常用颜色对应值：

| 颜色 | 十六进制 | 二进制说明 |
|------|---------|-----------|
| 白色 | `0xFFFF` | R=31, G=63, B=31 |
| 黑色 | `0x0000` | R=0,  G=0,  B=0  |
| 红色 | `0xF800` | R=31, G=0,  B=0  |
| 绿色 | `0x07E0` | R=0,  G=63, B=0  |
| 蓝色 | `0x001F` | R=0,  G=0,  B=31 |
| 黄色 | `0xFFE0` | R=31, G=63, B=0  |
| 青色 | `0x07FF` | R=0,  G=63, B=31 |
| 橙色 | `0xFC00` | R=31, G=32, B=0  |
| 灰色 | `0x8410` | R=16, G=32, B=16 |

---

## 七、常用汉字 GB2312 速查

| 汉字 | GB2312（\x转义） | 汉字 | GB2312（\x转义） |
|------|----------------|------|----------------|
| 中 | `\xD6\xD0` | 国 | `\xB9\xFA` |
| 你 | `\xC4\xE3` | 好 | `\xBA\xC3` |
| 汉 | `\xBA\xBA` | 字 | `\xD7\xD6` |
| 显 | `\xCF\xD4` | 示 | `\xCA\xBD` |
| 温 | `\xCE\xC2` | 度 | `\xB6\xC8` |
| 湿 | `\xCA\xAD` | 红 | `\xBA\xEC` |
| 色 | `\xC9\xAB` | 绿 | `\xC2\xCC` |
| 蓝 | `\xC0\xB6` | 第 | `\xB5\xDA` |
| 一 | `\xD2\xBB` | 二 | `\xB6\xFE` |
| 三 | `\xC8\xFD` | 行 | `\xD0\xD0` |

计算任意汉字的 GB2312 编码：在 Python 中运行 `"汉字".encode('gb2312').hex()`。

---

## 八、所需 Arduino 库

在 Arduino IDE 的库管理器中搜索安装：

| 库名 | 作者 | 用途 |
|------|------|------|
| Adafruit ST7789 | Adafruit | ST7789 驱动 |
| Adafruit GFX Library | Adafruit | 图形基础库（依赖） |
| Adafruit BusIO | Adafruit | 总线抽象层（依赖） |

板子配置（工具菜单）：
- Board：**ESP32S3 Dev Module**
- Flash Size：16MB
- PSRAM：OPI PSRAM（8MB）
- USB Mode：**Hardware CDC and JTAG**

---

## 九、调试速查

| 现象 | 原因 | 解决方法 |
|------|------|----------|
| 屏幕完全空白（背光不亮） | BLK 未接高电平 | `digitalWrite(GPIO5, HIGH)` |
| 屏幕有背光但全黑/全白 | 未调用 `invertDisplay(true)` | 在 `tft.init()` 后加 `tft.invertDisplay(true)` |
| 有颜色但汉字不显示 | `SPI.begin()` 破坏了软件SPI的GPIO控制 | 删除代码中所有 `SPI.begin()` 调用 |
| 字库读取返回全零 | CS2 或 MISO 接线错误 | 检查 GPIO9→CS2、GPIO13→FSO |
| 汉字显示为乱点阵 | GB2312 地址计算错误（漏掉 +846 偏移） | 检查 Zone 2 公式是否包含 `+ 846` |
| 字库读到数据但屏幕闪烁 | CS1 和 CS2 同时被拉低 | 检查每次操作前对方 CS 是否为 HIGH |
| 串口无输出 | USB CDC 未枚举完成 | 等待 `while(!Serial && ...)` 超时后自动继续 |
