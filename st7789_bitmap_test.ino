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

// ========== 测试图案1：实心方块（验证绘制方向）==========
const uint8_t BLOCK[32] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// ========== 测试图案2：十字（验证行列方向）==========
const uint8_t CROSS[32] = {
  0x00, 0x80,  // 第0行: 只有竖线
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x7F, 0xFE,  // 第7行: 横线
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80,
  0x00, 0x80
};

// ========== 测试图案3：带边框的"中"字（完整版）==========
const uint8_t ZHONG[32] = {
  0x00, 0x00,  // 空行
  0x00, 0x00,
  0x0F, 0xF0,  // 上横线 □□□□■■■■■■■■□□□□
  0x0C, 0x30,  // 左竖+右竖 □□□□■■□□□□■■□□□□
  0x0C, 0x30,
  0x0C, 0x30,
  0x0C, 0x30,
  0x0C, 0x30,
  0x0F, 0xF0,  // 中横线
  0x0C, 0x30,
  0x0C, 0x30,
  0x0C, 0x30,
  0x0C, 0x30,
  0x0C, 0x30,
  0x0F, 0xF0,  // 下横线
  0x00, 0x00
};

// ========== 绘制函数（与之前相同）==========
void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, 
                uint16_t color, uint16_t bg) {
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = bitmap[row * 2];
    uint8_t right = bitmap[row * 2 + 1];
    
    for (uint8_t bit = 0; bit < 8; bit++) {
      bool pixel = left & (0x80 >> bit);
      tft.drawPixel(x + bit, y + row, pixel ? color : bg);
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      bool pixel = right & (0x80 >> bit);
      tft.drawPixel(x + 8 + bit, y + row, pixel ? color : bg);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  
  // 第1个：左上角 - 实心白色方块（验证基本绘制）
  drawBitmap(10, 10, BLOCK, ST77XX_WHITE, ST77XX_BLACK);
  
  // 第2个：右上角 - 红色十字（确认行列方向）
  drawBitmap(190, 10, CROSS, ST77XX_RED, ST77XX_BLACK);
  
  // 第3个：中间 - 绿色"中"字（带边框完整版）
  drawBitmap(100, 100, ZHONG, ST77XX_GREEN, ST77XX_BLACK);
  
  // 第4个：左下角 - 蓝色放大的"中"字（2x）
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = ZHONG[row * 2];
    uint8_t right = ZHONG[row * 2 + 1];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (left & (0x80 >> bit)) {
        tft.fillRect(20 + bit*2, 160 + row*2, 2, 2, ST77XX_BLUE);
      }
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (right & (0x80 >> bit)) {
        tft.fillRect(20 + 16 + bit*2, 160 + row*2, 2, 2, ST77XX_BLUE);
      }
    }
  }
  
  // 第5个：右下角 - 黄色十字（2x）
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = CROSS[row * 2];
    uint8_t right = CROSS[row * 2 + 1];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (left & (0x80 >> bit)) {
        tft.fillRect(180 + bit*2, 160 + row*2, 2, 2, ST77XX_YELLOW);
      }
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (right & (0x80 >> bit)) {
        tft.fillRect(180 + 16 + bit*2, 160 + row*2, 2, 2, ST77XX_YELLOW);
      }
    }
  }
  
  Serial.println("检查屏幕：");
  Serial.println("1. 左上: 白色实心方块（16×16）");
  Serial.println("2. 右上: 红色十字（一横一竖）");
  Serial.println("3. 中间: 绿色'中'字（带边框的完整版）");
  Serial.println("4. 左下: 蓝色放大'中'字（32×32）");
  Serial.println("5. 右下: 黄色放大十字（32×32）");
}

void loop() {}
