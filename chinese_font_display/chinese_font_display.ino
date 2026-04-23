#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ── 针脚定义 ──────────────────────────────────
#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_MISO  14   // FSO 接 J1-20
#define TFT_BL     5
#define FONT_CS    9

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ── 字库读取 ──────────────────────────────────
void fontChipRead(uint32_t addr, uint8_t* buf, uint16_t len) {
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(FONT_CS, LOW);

  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >>  8) & 0xFF);
  SPI.transfer( addr        & 0xFF);

  for (uint16_t i = 0; i < len; i++) {
    buf[i] = SPI.transfer(0x00);
  }

  digitalWrite(FONT_CS, HIGH);
  SPI.endTransaction();
}

// ── GB2312 地址计算 ───────────────────────────
uint32_t getHanziAddr(uint8_t h, uint8_t l) {
  uint8_t  qu  = h - 0xA0;
  uint8_t  wei = l - 0xA0;
  return 0x2C9D0 + ((uint32_t)(qu - 1) * 94 + (wei - 1)) * 32;
}

// ── 绘制单个 16×16 汉字 ───────────────────────
void drawHanzi(int16_t x, int16_t y,
               const uint8_t* gb,
               uint16_t fg, uint16_t bg) {
  uint8_t buf[32];
  fontChipRead(getHanziAddr(gb[0], gb[1]), buf, 32);

  for (uint8_t row = 0; row < 16; row++) {
    uint16_t bits = ((uint16_t)buf[row * 2] << 8) | buf[row * 2 + 1];
    for (uint8_t col = 0; col < 16; col++) {
      tft.drawPixel(x + col, y + row,
                    (bits & (0x8000 >> col)) ? fg : bg);
    }
  }
}

// ── 绘制 GB2312 字符串 ────────────────────────
void drawChinese(int16_t x, int16_t y,
                 const char* str,
                 uint16_t fg, uint16_t bg) {
  int16_t cx = x;
  while (*str) {
    uint8_t ch = (uint8_t)*str;
    if (ch >= 0xA1 && *(str + 1)) {
      uint8_t gb[2] = { ch, (uint8_t)*(str + 1) };
      drawHanzi(cx, y, gb, fg, bg);
      cx  += 16;
      str += 2;
    } else {
      tft.drawChar(cx, y + 2, ch, fg, bg, 2);
      cx  += 12;
      str += 1;
    }
  }
}

// ── 字库自检 ──────────────────────────────────
bool testFont() {
  uint8_t buf[4];
  fontChipRead(getHanziAddr(0xD6, 0xD0), buf, 4);
  Serial.printf("[字库] %02X %02X %02X %02X\n",
                buf[0], buf[1], buf[2], buf[3]);
  bool ok = (buf[0] != 0x00 && buf[0] != 0xFF);
  Serial.println(ok ? "[字库] OK" : "[字库] 失败，检查CS2/FSO");
  return ok;
}

// ── setup ─────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL,  OUTPUT); digitalWrite(TFT_BL,  HIGH);
  pinMode(FONT_CS, OUTPUT); digitalWrite(FONT_CS, HIGH);

  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);  // CLK, MISO, MOSI

  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  if (!testFont()) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("Font Error!");
    tft.setTextSize(1);
    tft.setCursor(10, 130);
    tft.println("Check CS2(IO9)/FSO(IO14)");
    return;
  }

  // ?? 文件必须以 GB2312 编码保存
  drawChinese(4,  20, "你好世界",   0xFFFF, 0x0000);
  drawChinese(4,  44, "中文显示",   0x07FF, 0x0000);
  drawChinese(4,  68, "乐鑫ESP32",  0xFFE0, 0x0000);
  drawChinese(4,  92, "字库测试OK", 0x07E0, 0x0000);
}

void loop() {}