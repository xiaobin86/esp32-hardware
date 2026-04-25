// ============================================================
// ST7789 + GT30L32S4W 汉字显示
// 修复记录:
//   - Adafruit 5参数构造 = 软件SPI(bit-bang)，不能调用 SPI.begin()
//   - 字库读取改用软件SPI，完全隔离，不干扰屏幕
//   - IPS屏必须 invertDisplay(true)
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TFT_CS    10   // 屏幕片选  CS1
#define TFT_DC     6   // 数据/命令
#define TFT_RST    7   // 复位
#define TFT_MOSI  11   // SPI MOSI（屏幕+字库共用）
#define TFT_SCLK  12   // SPI CLK （屏幕+字库共用）
#define TFT_MISO  13   // SPI MISO（字库回读，屏幕不用）
#define TFT_BL     5   // 背光
#define FONT_CS    9   // 字库片选 CS2

// 5个参数 = 软件SPI(bit-bang)，永远不要对这几个pin调用 SPI.begin()！
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

static uint8_t  fontBuf[32];
static uint16_t pixBuf[16 * 16];

// ── GT30L32S4W 地址计算（来自官方datasheet 4.1.2节）────────
uint32_t gb2312Addr(uint8_t msb, uint8_t lsb) {
  if (msb >= 0xA1 && msb <= 0xA9 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xA1) * 94 + (lsb - 0xA1)) * 32 + 0x2C9D0;
  if (msb >= 0xB0 && msb <= 0xF7 && lsb >= 0xA1)
    return ((uint32_t)(msb - 0xB0) * 94 + (lsb - 0xA1) + 846) * 32 + 0x2C9D0;
  return 0;
}

// ── 软件SPI：向字库发送1字节（上升沿锁存，MODE0）───────────
void fontSend(uint8_t dat) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(TFT_SCLK, LOW);
    digitalWrite(TFT_MOSI, (dat & 0x80) ? HIGH : LOW);
    dat <<= 1;
    digitalWrite(TFT_SCLK, HIGH);
  }
  digitalWrite(TFT_SCLK, LOW);  // 时钟回到空闲低电平
}

// ── 软件SPI：从字库读取1字节（下降沿输出，空闲期采样）─────
// 参考厂商C51示例代码时序：SCK降→采样MISO→SCK升
uint8_t fontRecv() {
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(TFT_SCLK, HIGH);
    digitalWrite(TFT_SCLK, LOW);                          // 下降沿触发字库输出
    data = (data << 1) | digitalRead(TFT_MISO);           // 采样
  }
  return data;
}

// ── 从字库读取32字节 ────────────────────────────────────────
bool fontRead(uint32_t addr) {
  digitalWrite(TFT_CS,  HIGH);   // 确保屏幕CS不干扰
  digitalWrite(TFT_SCLK, LOW);  // 时钟空闲低

  digitalWrite(FONT_CS, LOW);   // 选中字库芯片
  delayMicroseconds(2);

  fontSend(0x03);                        // READ指令
  fontSend((addr >> 16) & 0xFF);
  fontSend((addr >>  8) & 0xFF);
  fontSend( addr        & 0xFF);

  for (int i = 0; i < 32; i++) fontBuf[i] = fontRecv();

  digitalWrite(FONT_CS, HIGH);   // 释放字库
  delayMicroseconds(2);

  for (int i = 0; i < 32; i++) if (fontBuf[i]) return true;
  return false;
}

// ── 绘制单个汉字（16x16像素窗口，字体实为15x16有效列）──────
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

// ── 显示GB2312字符串（汉字用\xHH转义，支持ASCII和\n）───────
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
  Serial.println("\n===== ST7789 汉字显示 =====");

  // GPIO 初始化
  pinMode(TFT_BL,   OUTPUT); digitalWrite(TFT_BL,   HIGH);
  pinMode(FONT_CS,  OUTPUT); digitalWrite(FONT_CS,  HIGH);
  pinMode(TFT_MISO, INPUT);  // MISO只读，不初始化为输出

  // ── 阶段1：屏幕初始化 ──────────────────────────────────
  tft.init(240, 240);
  tft.setRotation(0);
  tft.invertDisplay(true);   // IPS屏必须，否则颜色全黑/反色

  // 红色全屏：验证屏幕和invertDisplay是否生效
  tft.fillScreen(0xF800);
  Serial.println("[1] 红色全屏 - 能看到说明屏幕OK，等2秒...");
  delay(2000);

  // ── 阶段2：字库读取验证 ────────────────────────────────
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setTextSize(1);
  tft.setCursor(0, 0);

  uint32_t addr = gb2312Addr(0xD6, 0xD0);  // "中"
  Serial.printf("[2] '中' 地址: 0x%06X\n", addr);
  tft.printf("Font addr: 0x%06X\n", addr);

  bool ok = fontRead(addr);
  Serial.printf("[2] 字库读取: %s\n", ok ? "成功" : "失败！检查CS2(GPIO9)和MISO(GPIO13)");
  tft.printf("Font read: %s\n", ok ? "OK" : "FAIL");

  if (ok) {
    printBitmap("中");
    // 在屏幕中间画"中"字
    drawHanzi(112, 80, 0xD6, 0xD0, 0xFFFF, 0x0000);
    tft.setCursor(90, 100);
    tft.setTextSize(2);
    tft.print("<- zhong");
    Serial.println("[2] 已在屏幕中间显示\"中\"字");
  } else {
    Serial.println("     排查：CS2接GPIO9？FSO接GPIO13？CS1空闲时是否HIGH？");
    while (true) delay(1000);  // 停在这里等排查
  }

  delay(2000);

  // ── 阶段3：完整汉字显示 ────────────────────────────────
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

  // 温度:25.6C  湿度:68%
  drawStr(10, 76,
    "\xCE\xC2\xB6\xC8"   // 温度
    ":25.6C\n"
    "\xCA\xAD\xB6\xC8"   // 湿度
    ":68%",
    0x07FF, 0x0000);

  // 彩色：红色 绿色 蓝色
  drawStr(10, 116, "\xBA\xEC\xC9\xAB", 0xF800, 0x0000);   // 红色
  drawStr(58, 116, "\xC2\xCC\xC9\xAB", 0x07E0, 0x0000);   // 绿色
  drawStr(106,116, "\xC0\xB6\xC9\xAB", 0x001F, 0x0000);   // 蓝色

  // 多行测试
  drawStr(10, 140,
    "\xB5\xDA\xD2\xBB\xD0\xD0\n"  // 第一行
    "\xB5\xDA\xB6\xFE\xD0\xD0\n"  // 第二行
    "\xB5\xDA\xC8\xFD\xD0\xD0",   // 第三行
    0xFC00, 0x0000);

  tft.drawFastHLine(0, 202, 240, 0x8410);
  tft.setTextSize(1);
  tft.setTextColor(0x8410, 0x0000);
  tft.setCursor(10, 208);
  tft.print("GT30L32S4W  GB2312  16x16");
  tft.setCursor(10, 220);
  tft.print("ESP32-S3 + ST7789V  ZJY154S10Z0TG01");

  Serial.println("[3] 完成！");
}

void loop() { delay(1000); }
