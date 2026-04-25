// ============================================================
// TFT_eSPI + GT30L32S4W 汉字显示（硬件 SPI 共享总线版）
//
// 核心设计：
//   TFT_eSPI 和字库芯片共享 ESP32-S3 的 FSPI 总线
//   屏幕 CS=GPIO10，字库 CS=GPIO9
//   每次操作通过 SPI.beginTransaction 切换设备参数
//
// 关键修复：
//   1. pixBuf 必须 4 字节对齐（ESP32 DMA 要求）
//   2. 去掉 USE_HSPI_PORT，确保 TFT_eSPI 和字库都用 FSPI
//   3. SPI.begin() 先于 tft.init()，确保 MISO 配置正确
// ============================================================

#include <TFT_eSPI.h>
#include <SPI.h>

// ── 引脚定义 ────────────────────────────────────────────────
#define TFT_CS    10   // 屏幕片选（与 User_Setup.h 一致）
#define FONT_CS    9   // 字库片选

// ── 创建 TFT_eSPI 对象（硬件 SPI，引脚在 User_Setup.h 中配置）──
TFT_eSPI tft = TFT_eSPI();

// ── 字库点阵缓冲 ────────────────────────────────────────────
static uint8_t  fontBuf[32];
// ⚠️ 关键：pushPixels 要求 4 字节对齐（ESP32 DMA 32-bit 访问要求）
static uint16_t pixBuf[16 * 16] __attribute__((aligned(4)));

// ── GT30L32S4W 地址计算 ─────────────────────────────────────
uint32_t gb2312Addr(uint8_t msb, uint8_t lsb) {
  if (msb >= 0xA1 && msb <= 0xA9 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xA1) * 94 + (lsb - 0xA1)) * 32 + 0x2C9D0;
  if (msb >= 0xB0 && msb <= 0xF7 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xB0) * 94 + (lsb - 0xA1) + 846) * 32 + 0x2C9D0;
  return 0;
}

// ── 从字库读取 32 字节（硬件 SPI）────────────────────────────
// 与 TFT_eSPI 共享 SPI 总线，通过 beginTransaction 切换
bool fontRead(uint32_t addr) {
  // 防御性拉高屏幕 CS（确保屏幕不参与字库通信）
  digitalWrite(TFT_CS, HIGH);

  // 将 SPI 外设配置为字库参数（8MHz, MODE0）
  SPI.beginTransaction(SPISettings(8000000UL, MSBFIRST, SPI_MODE0));
  digitalWrite(FONT_CS, LOW);

  SPI.transfer(0x03);                      // READ 指令
  SPI.transfer((addr >> 16) & 0xFF);       // 地址高字节
  SPI.transfer((addr >>  8) & 0xFF);       // 地址中字节
  SPI.transfer( addr        & 0xFF);       // 地址低字节

  // 发送 dummy byte 驱动时钟，同时读取字库返回的数据
  for (int i = 0; i < 32; i++) {
    fontBuf[i] = SPI.transfer(0x00);
  }

  digitalWrite(FONT_CS, HIGH);
  SPI.endTransaction();  // 释放 SPI 总线

  // 全零检测：正常汉字点阵不会全零
  for (int i = 0; i < 32; i++) if (fontBuf[i]) return true;
  return false;
}

// ── 绘制单个汉字 ─────────────────────────────────────────────
void drawHanzi(int16_t x, int16_t y, uint8_t msb, uint8_t lsb,
               uint16_t fg, uint16_t bg) {
  uint32_t addr = gb2312Addr(msb, lsb);
  if (!addr || !fontRead(addr)) return;

  // 将 1bit 点阵解码为 RGB565 像素缓冲
  for (int row = 0; row < 16; row++) {
    uint8_t b0 = fontBuf[row * 2];
    uint8_t b1 = fontBuf[row * 2 + 1];
    for (int col = 0; col < 8; col++) {
      pixBuf[row * 16 + col]     = (b0 & (0x80 >> col)) ? fg : bg;
      pixBuf[row * 16 + 8 + col] = (b1 & (0x80 >> col)) ? fg : bg;
    }
  }

  // pushImage 内部自动处理 SPI 事务（begin_tft_write/end_tft_write）
  // 不需要外部包裹 startWrite/endWrite
  tft.pushImage(x, y, 16, 16, pixBuf);
}

// ── 显示 GB2312 字符串（\xHH 转义，支持 ASCII 混排和 \n）────────
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
  Serial.println("\n===== TFT_eSPI 汉字显示（硬件SPI共享总线）=====");

  // 字库 CS 初始化（默认高电平 = 未选中）
  pinMode(FONT_CS, OUTPUT);
  digitalWrite(FONT_CS, HIGH);

  // ── 显式初始化硬件 SPI（FSPI）────────────────────────────
  // 必须先于 tft.init() 调用！确保 GPIO11/12/13 配置为 SPI 功能，
  // 特别是 GPIO13(MISO) 必须配置为输入，否则字库数据无法回读。
  SPI.begin();

  // ── TFT_eSPI 初始化 ────────────────────────────────────
  // 内部也会调用 SPI.begin()，但 SPI 已初始化则跳过重复配置
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  Serial.println("[INIT] TFT_eSPI 初始化完成");

  // ── 阶段1：红色全屏验证 ─────────────────────────────────
  tft.fillScreen(TFT_RED);
  Serial.println("[1] 红色全屏 OK，等 1 秒...");
  delay(1000);

  // ── 阶段2：验证字库读取 ─────────────────────────────────
  tft.fillScreen(TFT_BLACK);
  uint32_t addr = gb2312Addr(0xD6, 0xD0);  // "中"
  Serial.printf("[2] '中' 地址: 0x%06X\n", addr);

  bool ok = fontRead(addr);
  Serial.printf("[2] 字库读取: %s\n", ok ? "成功" : "失败！");

  if (!ok) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 100); tft.setTextSize(2);
    tft.print("FONT FAIL");
    while (true) delay(1000);
  }

  printBitmap("中");
  drawHanzi(112, 80, 0xD6, 0xD0, TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(80, 100); tft.print("TFT_eSPI OK");
  Serial.println("[2] 字库读取成功，屏幕中心已显示'中'");
  delay(1000);

  // ── 阶段3：完整汉字界面 ─────────────────────────────────
  tft.fillScreen(TFT_BLACK);

  // 标题
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 4);
  tft.print("TFT_eSPI Demo");
  tft.drawFastHLine(0, 22, 240, TFT_WHITE);

  // 中国 你好
  drawStr(10, 28,
    "\xD6\xD0\xB9\xFA"   // 中国
    "  "
    "\xC4\xE3\xBA\xC3",  // 你好
    TFT_WHITE, TFT_BLACK);

  // ESP32汉字显示
  drawStr(10, 52,
    "ESP32"
    "\xBA\xBA\xD7\xD6"   // 汉字
    "\xCF\xD4\xCA\xBD",  // 显示
    TFT_YELLOW, TFT_BLACK);

  // 硬件SPI版本
  drawStr(10, 76,
    "\xD3\xB2\xBE\xFE"   // 硬件
    "SPI"
    "\xB0\xE6\xB1\xBE",  // 版本
    TFT_CYAN, TFT_BLACK);

  // 温度湿度
  drawStr(10, 100,
    "\xCE\xC2\xB6\xC8"   // 温度
    ":25.6C\n"
    "\xCA\xAD\xB6\xC8"   // 湿度
    ":68%",
    TFT_CYAN, TFT_BLACK);

  // 彩色汉字
  drawStr(10,  136, "\xBA\xEC\xC9\xAB", TFT_RED,    TFT_BLACK);  // 红色
  drawStr(58,  136, "\xC2\xCC\xC9\xAB", TFT_GREEN,  TFT_BLACK);  // 绿色
  drawStr(106, 136, "\xC0\xB6\xC9\xAB", TFT_BLUE,   TFT_BLACK);  // 蓝色
  drawStr(154, 136, "\xBB\xC6\xC9\xAB", TFT_YELLOW, TFT_BLACK);  // 黄色

  // 多行
  drawStr(10, 160,
    "\xB5\xDA\xD2\xBB\xD0\xD0\n"   // 第一行
    "\xB5\xDA\xB6\xFE\xD0\xD0\n"   // 第二行
    "\xB5\xDA\xC8\xFD\xD0\xD0",    // 第三行
    TFT_ORANGE, TFT_BLACK);

  tft.drawFastHLine(0, 202, 240, TFT_DARKGREY);
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(10, 208);
  tft.print("TFT_eSPI + Shared SPI Bus");
  tft.setCursor(10, 220);
  tft.print("ESP32-S3 + ST7789V + GT30L32S4W");

  Serial.println("[3] 完成！");
}

void loop() { delay(1000); }
