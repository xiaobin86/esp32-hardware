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

// ========== "中"字 16×16 点阵数据（直接硬编码）==========
// 1 = 亮点，0 = 暗点
// 每行2字节，共16行 = 32字节
const uint8_t ZHONG[32] = {
  0x00, 0x80,  // 第1行: 00000000 10000000
  0x00, 0x80,  // 第2行: 00000000 10000000
  0x00, 0x80,  // 第3行: 00000000 10000000
  0x00, 0x80,  // 第4行: 00000000 10000000
  0x00, 0x80,  // 第5行: 00000000 10000000
  0x7F, 0xFE,  // 第6行: 01111111 11111110  ← 中间横线
  0x00, 0x80,  // 第7行
  0x00, 0x80,  // 第8行
  0x00, 0x80,  // 第9行
  0x00, 0x80,  // 第10行
  0x7F, 0xFE,  // 第11行: 01111111 11111110  ← 中间横线
  0x00, 0x80,  // 第12行
  0x00, 0x80,  // 第13行
  0x00, 0x80,  // 第14行
  0x00, 0x80,  // 第15行
  0x00, 0x80   // 第16行
};

// ========== 绘制硬编码点阵 ==========
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
  
  // 开启背光
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // 初始化屏幕
  tft.init(240, 240);
  tft.setRotation(0);
  
  // 清屏黑色
  tft.fillScreen(ST77XX_BLACK);
  
  // 画几个测试图案确认屏幕正常
  tft.fillRect(0, 0, 50, 50, ST77XX_RED);      // 左上角红色方块
  tft.fillRect(190, 0, 50, 50, ST77XX_GREEN);  // 右上角绿色方块
  tft.fillRect(0, 190, 50, 50, ST77XX_BLUE);   // 左下角蓝色方块
  
  // 绘制"中"字（硬编码点阵）
  drawBitmap(100, 100, ZHONG, ST77XX_WHITE, ST77XX_BLACK);
  
  // 再画一个放大的（每个像素画成2x2）
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t left  = ZHONG[row * 2];
    uint8_t right = ZHONG[row * 2 + 1];
    
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (left & (0x80 >> bit)) {
        tft.fillRect(150 + bit*2, 100 + row*2, 2, 2, ST77XX_YELLOW);
      }
    }
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (right & (0x80 >> bit)) {
        tft.fillRect(150 + 16 + bit*2, 100 + row*2, 2, 2, ST77XX_YELLOW);
      }
    }
  }
  
  Serial.println("绘制完成！检查屏幕:");
  Serial.println("1. 左上角应该有红色方块");
  Serial.println("2. 右上角应该有绿色方块");
  Serial.println("3. 左下角应该有蓝色方块");
  Serial.println("4. 中间应该有白色'中'字（16×16）");
  Serial.println("5. 右边应该有黄色放大'中'字（32×32）");
}

void loop() {
  // 空循环
}
