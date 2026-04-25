// ============================================
// TFT_eSPI User_Setup.h - ESP32-S3 修复版
// ============================================
// 关键修复：
// 1. #define USE_HSPI_PORT - 绕过 ESP32-S3 FSPI bug（必须！）
// 2. ST7789 反色设置
// 3. 降低 SPI 频率到 10MHz 测试
// ============================================
// 重要：必须复制到 Arduino 库文件夹！
// 路径：Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
// ============================================

// 驱动芯片选择
#define ST7789_DRIVER
// #define ST7789_2_DRIVER  // 如果 ST7789_DRIVER 不行，尝试这个

// 屏幕分辨率
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// SPI 引脚（ESP32-S3）
#define TFT_MISO  13
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_CS    10
#define TFT_DC     6
#define TFT_RST    7

// ============================================
// 关键修复：ESP32-S3 必须使用 HSPI！
// ============================================
// ESP32-S3 的 Arduino Core (>2.0.14) 有 FSPI bug：
// FSPI 被定义为 0，导致 REG_SPI_BASE 返回 NULL
// 不定义这行会导致 tft.init() 崩溃/死锁
#define USE_HSPI_PORT

// ============================================
// ST7789 特殊配置
// ============================================
// 反色（ST7789 通常需要，如果白屏/黑屏尝试切换）
#define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

// 颜色顺序（如果颜色反了，改成 TFT_BGR）
#define TFT_RGB_ORDER TFT_RGB
// #define TFT_RGB_ORDER TFT_BGR

// ============================================
// SPI 频率
// ============================================
// 先从低频率开始测试
#define SPI_FREQUENCY  10000000   // 10MHz（保守值）
// #define SPI_FREQUENCY  27000000  // 27MHz

#define SPI_READ_FREQUENCY  8000000

// ============================================
// 字体配置
// ============================================
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ============================================
// 其他
// ============================================
#define SUPPORT_TRANSACTIONS

// 调试（可选）
// #define TFT_DEBUG
