# TFT_eSPI + GT30L32S4W 字库中文显示方案

## 方案优势

相比 Adafruit_ST7789，TFT_eSPI 有以下优势：

1. **性能更好**：支持 ESP32-S3 DMA，刷新速度更快
2. **SPI 管理更灵活**：支持多 SPI 实例（HSPI/VSPI），字库和屏幕可完全独立
3. **底层控制更强**：`pushImage` 等高效绘制方法
4. **社区活跃**：持续更新，中文支持案例多
5. **字库方案多样**：
   - 继续使用外置 GT30L32S4W 字库芯片（本方案）
   - 或加载 .vlw 字体文件到 SPIFFS（无需字库芯片）

---

## 安装步骤

### 1. 安装 TFT_eSPI 库

Arduino IDE → 工具 → 管理库 → 搜索 **"TFT_eSPI"** → 安装（by Bodmer）

### 2. 配置 User_Setup.h

复制 `User_Setup.h` 到库文件夹：
```
Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
```

或编辑现有文件，确保以下配置：
```cpp
#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#define TFT_MISO  13
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7
```

### 3. 上传代码

打开 `chinese_font_display_tft_espi.ino`，选择 **ESP32-S3 DevKitC-1**，上传。

---

## 核心改进点

### 1. 独立 SPI 实例

```cpp
SPIClass fontSPI(HSPI);  // 字库使用 HSPI
// TFT_eSPI 自动使用 VSPI/FSPI
```

字库和屏幕使用**完全独立的 SPI 总线**，互不干扰！

### 2. 无需重新初始化 SPI

```cpp
// 之前（Adafruit）的麻烦操作：
SPI.end();
SPI.begin(...);
// ...读字库...
SPI.end();
SPI.begin(...);

// 现在（TFT_eSPI）：
fontSPI.begin(...);
// ...读字库...
fontSPI.end();
// TFT_eSPI 完全不受影响！
```

### 3. 高效绘制

```cpp
// pushImage 直接推送像素行，比 drawPixel 快 10 倍以上
uint16_t lineBuffer[16];
tft.pushImage(x, y + row, 16, 1, lineBuffer);
```

---

## 接线（与之前相同）

| 功能 | GPIO | 说明 |
|------|------|------|
| TFT_CS | 10 | 屏幕片选 |
| TFT_DC | 6 | 数据/命令 |
| TFT_RST | 7 | 复位 |
| TFT_MOSI | 11 | SPI 数据（共用）|
| TFT_SCLK | 12 | SPI 时钟（共用）|
| TFT_MISO | 13 | 字库数据回读 |
| FONT_CS | 9 | 字库片选 |
| TFT_BL | 5 | 背光 |

---

## 故障排查

### 如果仍不显示汉字

1. **检查串口输出**
   - 地址是否正确（0x4F670、0x3A7B0 等）
   - 点阵数据是否匹配预期汉字

2. **检查 User_Setup.h**
   - 是否复制到正确位置
   - 引脚是否与实际接线一致

3. **尝试 drawPixel 版本**
   ```cpp
   // 在 setup() 中将 drawHanzi 替换为 drawHanziPixel
   drawHanziPixel(50, 50, TFT_WHITE, TFT_BLACK, hz_zhong);
   ```

4. **检查字库芯片**
   - CS2(GPIO9) 是否有信号
   - 用示波器看 SPI 波形

---

## 备选方案：无字库芯片

如果你的屏幕**没有字库芯片**，或字库芯片损坏，可以用 TFT_eSPI 的 .vlw 字体：

### 步骤

1. **生成字体文件**
   - 下载 Processing IDE（https://processing.org/）
   - 打开 TFT_eSPI 库 Tools 目录下的 `Create_Smooth_Font` 草图
   - 选择中文字体（如微软雅黑）
   - 选择需要的字符（如"中国你好"）
   - 生成 .vlw 文件

2. **上传字体**
   ```cpp
   // 将 .vlw 文件放到 SPIFFS
   tft.loadFont("MicrosoftYaHei-24");
   tft.setTextColor(TFT_WHITE, TFT_BLACK);
   tft.drawString("中国你好", 50, 50);
   tft.unloadFont();
   ```

**缺点**：需要生成字体文件，占用 Flash 空间。
**优点**：无需外置字库芯片，显示更灵活。

---

## 关联文档

- [GT30L32S4W_字库芯片技术详解.md](D:\acela\my_wiki\我的知识库\raw\esp32-hardware\st7789\GT30L32S4W_字库芯片技术详解.md)
- [MY_HARDWARE.md](D:\work\esp32-demo\MY_HARDWARE.md)

---

*最后更新：2026-04-24*
