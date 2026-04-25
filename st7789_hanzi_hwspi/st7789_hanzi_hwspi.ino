// ============================================================
// ST7789 + GT30L32S4W 汉字显示（硬件 SPI 版）
//
// 与软件SPI版的区别：
//   Adafruit 3参数构造 → 使用 ESP32-S3 硬件 SPI 外设(FSPI)
//   屏幕刷新速度大幅提升（~40MHz vs 软件SPI的~1MHz）
//   字库读取同样走硬件 SPI，用 beginTransaction 切换参数
//
// 为何不冲突：
//   ESP32-S3 默认 FSPI 引脚 = SCK:12 MOSI:11 MISO:13 SS:10
//   与本项目接线完全一致，SPI.begin() 自动包含 MISO
//   屏幕(CS1=GPIO10) 和字库(CS2=GPIO9) 靠不同 CS 分时复用总线
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ── 引脚定义 ────────────────────────────────────────────────
#define TFT_CS    10   // 屏幕片选  CS1
#define TFT_DC     6   // 数据/命令
#define TFT_RST    7   // 复位
#define TFT_MOSI  11   // SPI MOSI（ESP32-S3 FSPI 默认）
#define TFT_SCLK  12   // SPI CLK （ESP32-S3 FSPI 默认）
#define TFT_MISO  13   // SPI MISO（ESP32-S3 FSPI 默认，字库回读）
#define TFT_BL     5   // 背光
#define FONT_CS    9   // 字库片选 CS2

// ── 3参数构造 = 硬件SPI，使用 SPI 全局对象 ─────────────────
// 注意：不传 MOSI/SCLK，Adafruit 通过 SPI.begin() 获得引脚配置
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ── 屏幕 SPI 参数（写屏用高速，字库用保守速度）──────────────
// Adafruit 内部默认用 24MHz，这里显式设置方便调整
#define TFT_SPI_FREQ   40000000UL   // 40MHz 写屏
#define FONT_SPI_FREQ   8000000UL   // 8MHz 读字库（GT30L32S4W 最高 80MHz，保守起见用8M）

static uint8_t  fontBuf[32];
static uint16_t pixBuf[16 * 16];

// ── GT30L32S4W 地址计算（datasheet 4.1.2节，15×16点阵）──────
uint32_t gb2312Addr(uint8_t msb, uint8_t lsb) {
  if (msb >= 0xA1 && msb <= 0xA9 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xA1) * 94 + (lsb - 0xA1)) * 32 + 0x2C9D0;
  if (msb >= 0xB0 && msb <= 0xF7 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xB0) * 94 + (lsb - 0xA1) + 846) * 32 + 0x2C9D0;
  return 0;
}

// ── 从字库读取32字节（硬件SPI）──────────────────────────────
// 关键：用 beginTransaction 切换到字库的 SPI 参数（MODE0, 8MHz）
// Adafruit 也用 beginTransaction 切换到屏幕参数，两者轮流使用总线
bool fontRead(uint32_t addr) {
  // 确保屏幕 CS 为高（Adafruit endWrite 后已是高，这里防御性写一次）
  digitalWrite(TFT_CS, HIGH);

  SPI.beginTransaction(SPISettings(FONT_SPI_FREQ, MSBFIRST, SPI_MODE0));
  digitalWrite(FONT_CS, LOW);

  SPI.transfer(0x03);                      // READ 指令
  SPI.transfer((addr >> 16) & 0xFF);       // 地址高字节
  SPI.transfer((addr >>  8) & 0xFF);       // 地址中字节
  SPI.transfer( addr        & 0xFF);       // 地址低字节
  for (int i = 0; i < 32; i++) fontBuf[i] = SPI.transfer(0x00);

  digitalWrite(FONT_CS, HIGH);
  SPI.endTransaction();

  for (int i = 0; i < 32; i++) if (fontBuf[i]) return true;
  return false;  // 全零 = 接线错误或地址越界
}

// ── 绘制单个汉字 ─────────────────────────────────────────────
void drawHanzi(int16_t x, int16_t y, uint8_t msb, uint8_t lsb,
               uint16_t fg, uint16_t bg) {
  uint32_t addr = gb2312Addr(msb, lsb);
  if (!addr || !fontRead(addr)) return;

  // 将1bit点阵转为RGB565像素缓冲
  for (int row = 0; row < 16; row++) {
    uint8_t b0 = fontBuf[row * 2];
    uint8_t b1 = fontBuf[row * 2 + 1];
    for (int col = 0; col < 8; col++) {
      pixBuf[row * 16 + col]     = (b0 & (0x80 >> col)) ? fg : bg;
      pixBuf[row * 16 + 8 + col] = (b1 & (0x80 >> col)) ? fg : bg;
    }
  }
  // 一次性写入整个16×16区域，内部使用硬件SPI的 startWrite/endWrite
  tft.drawRGBBitmap(x, y, pixBuf, 16, 16);
}

// ── 显示GB2312字符串（\xHH转义，支持ASCII混排和\n换行）───────
void drawStr(int16_t x, int16_t y, const char* str,
             uint16_t fg, uint16_t bg) {
  int16_t cx = x, cy = y;
  while (*str) {
    uint8_t c = (uint8_t)*str;
    if (c == '\n') { cx = x; cy += 18; str++; continue; }
    if (c >= 0x80) {
      uint8_t lsb = (uint8_t)*(str + 1);
      if (!lsb) break;
      if (cx + 16 > 240) { cx = x; cy += 18; }
      if (cy + 16 > 240) break;
      drawHanzi(cx, cy, c, lsb, fg, bg);
      cx += 16; str += 2;
    } else {
      if (cx + 6 > 240) { cx = x; cy += 18; }
      tft.setTextSize(1);
      tft.setTextColor(fg, bg);
      tft.setCursor(cx, cy + 4);
      tft.print((char)c);
      cx += 6; str++;
    }
  }
}

// ── 串口打印点阵（调试用）───────────────────────────────────
void printBitmap(const char* label) {
  Serial.printf("[%s]\n", label);
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 8; col++)
      Serial.print((fontBuf[row*2]   & (0x80>>col)) ? "█" : "░");
    for (int col = 0; col < 8; col++)
      Serial.print((fontBuf[row*2+1] & (0x80>>col)) ? "█" : "░");
    Serial.println();
  }
}

// ────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);
  Serial.println("\n===== ST7789 汉字显示（硬件SPI版）=====");

  // GPIO 初始化
  pinMode(TFT_BL,  OUTPUT); digitalWrite(TFT_BL,  HIGH);  // 背光开
  pinMode(FONT_CS, OUTPUT); digitalWrite(FONT_CS, HIGH);  // 字库默认未选中

  // 硬件 SPI 初始化
  // ESP32-S3 默认 FSPI 引脚：SCK=12, MOSI=11, MISO=13, SS=10
  // 与接线完全一致，SPI.begin() 无参数调用即可正确配置 MISO
  SPI.begin();

  // 屏幕初始化（3参数构造，使用上面已初始化的 SPI 对象）
  tft.init(240, 240, SPI_MODE0);  // 显式传 SPI_MODE0，与字库时序一致
  tft.setRotation(0);
  tft.invertDisplay(true);        // IPS 屏必须，否则颜色全反

  // ── 阶段1：红色全屏验证屏幕 ────────────────────────────
  tft.fillScreen(0xF800);
  Serial.println("[1] 红色全屏 OK，等 1 秒...");
  delay(1000);

  // ── 阶段2：验证字库读取 ─────────────────────────────────
  tft.fillScreen(0x0000);
  uint32_t addr = gb2312Addr(0xD6, 0xD0);  // "中"
  Serial.printf("[2] '中' 地址: 0x%06X\n", addr);

  bool ok = fontRead(addr);
  Serial.printf("[2] 字库读取: %s\n", ok ? "成功" : "失败！检查CS2(GPIO9)和MISO(GPIO13)");

  if (!ok) {
    tft.setTextColor(0xF800, 0x0000);
    tft.setCursor(10, 100); tft.setTextSize(2);
    tft.print("FONT FAIL");
    Serial.println("排查：CS2→GPIO9？FSO→GPIO13？CS1空闲是否HIGH？");
    while (true) delay(1000);
  }

  printBitmap("中");
  drawHanzi(112, 80, 0xD6, 0xD0, 0xFFFF, 0x0000);
  tft.setTextSize(1); tft.setTextColor(0x07FF, 0x0000);
  tft.setCursor(80, 100); tft.print("HW SPI OK");
  Serial.println("[2] 字库读取成功，屏幕中心已显示\"中\"");
  delay(1000);

  // ── 阶段3：完整汉字界面 ────────────────────────────────
  tft.fillScreen(0x0000);
  tft.setTextSize(2);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setCursor(10, 4);
  tft.print("Hanzi Demo");
  tft.drawFastHLine(0, 22, 240, 0xFFFF);

  // 中国 你好
  drawStr(10, 28,
    "\xD6\xD0\xB9\xFA"   // 中国
    "  "
    "\xC4\xE3\xBA\xC3",  // 你好
    0xFFFF, 0x0000);

  // ESP32汉字显示
  drawStr(10, 52,
    "ESP32"
    "\xBA\xBA\xD7\xD6"   // 汉字
    "\xCF\xD4\xCA\xBD",  // 显示
    0xFFE0, 0x0000);

  // 硬件SPI版本
  drawStr(10, 76,
    "\xD3\xB2\xBE\xFE"   // 硬件
    "SPI"
    "\xB0\xE6\xB1\xBE",  // 版本
    0x07FF, 0x0000);

  // 温度:25.6C  湿度:68%
  drawStr(10, 100,
    "\xCE\xC2\xB6\xC8"   // 温度
    ":25.6C\n"
    "\xCA\xAD\xB6\xC8"   // 湿度
    ":68%",
    0x07FF, 0x0000);

  // 彩色汉字
  drawStr(10,  136, "\xBA\xEC\xC9\xAB", 0xF800, 0x0000);  // 红色
  drawStr(58,  136, "\xC2\xCC\xC9\xAB", 0x07E0, 0x0000);  // 绿色
  drawStr(106, 136, "\xC0\xB6\xC9\xAB", 0x001F, 0x0000);  // 蓝色
  drawStr(154, 136, "\xBB\xC6\xC9\xAB", 0xFFE0, 0x0000);  // 黄色

  // 多行
  drawStr(10, 160,
    "\xB5\xDA\xD2\xBB\xD0\xD0\n"   // 第一行
    "\xB5\xDA\xB6\xFE\xD0\xD0\n"   // 第二行
    "\xB5\xDA\xC8\xFD\xD0\xD0",    // 第三行
    0xFC00, 0x0000);

  tft.drawFastHLine(0, 202, 240, 0x8410);
  tft.setTextSize(1);
  tft.setTextColor(0x8410, 0x0000);
  tft.setCursor(10, 208);
  tft.print("HW SPI @ 40MHz  Font @ 8MHz");
  tft.setCursor(10, 220);
  tft.print("ESP32-S3 + ST7789V + GT30L32S4W");

  Serial.println("[3] 完成！");
}

void loop() { delay(1000); }
