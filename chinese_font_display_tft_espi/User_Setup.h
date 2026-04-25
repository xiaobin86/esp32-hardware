// ============================================
// TFT_eSPI User_Setup.h - ESP32-S3 + ST7789 240x240
// ============================================
// 硬件：ESP32-S3-DevKitC-1 + ZJY154S10Z0TG01（ST7789 IPS 带字库版）
// 接线：GPIO10(CS) 11(MOSI) 12(SCK) 13(MISO) 6(DC) 7(RST) 5(BL)
//
// ⚠️ 重要：必须将此文件复制到 Arduino 库目录下！
// 路径：Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
// ============================================

// ========== 驱动芯片选择 ==========
// ST7789 240x240 IPS 显示屏
#define ST7789_DRIVER
// #define ST7789_2_DRIVER  // 备用：如果 ST7789_DRIVER 颜色异常，尝试此选项

// ========== 屏幕分辨率 ==========
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// ========== SPI 引脚（ESP32-S3 FSPI 默认引脚）==========
// 与字库芯片 GT30L32S4W 共享总线（GPIO11/12/13）
#define TFT_MISO  13   // FSO（字库数据回读，屏幕只写不用）
#define TFT_MOSI  11   // SDA（屏幕+字库共用 MOSI）
#define TFT_SCLK  12   // SCL（屏幕+字库共用 SCK）
#define TFT_CS    10   // CS1（屏幕片选）
#define TFT_DC     6   // DC（数据/命令选择）
#define TFT_RST    7   // RES（复位）

// ========== 背光控制 ==========
#define TFT_BL     5            // BLK（背光控制引脚）
#define TFT_BACKLIGHT_ON HIGH   // HIGH = 背光开启

// ========== SPI 外设选择（关键！共享总线不能开 HSPI）==========
// 
// ESP32-S3 有两个用户 SPI 外设：
//   - FSPI (SPI2)：Arduino 默认 SPI 全局对象
//   - HSPI (SPI3)：备用外设
// 
// 本方案中 TFT_eSPI 和字库芯片共用总线，必须统一用同一个外设。
// TFT_eSPI 默认用 FSPI，代码中字库读取也用 SPI(FSPI)，两者匹配。
// 
// ⚠️ 如果定义 USE_HSPI_PORT，TFT_eSPI 会切换到 HSPI，
//    但字库代码仍用 FSPI，导致 GPIO 控制权冲突、读取全零！
//
// #define USE_HSPI_PORT  // ← 不要开启！共享总线方案禁用

// ========== ST7789 IPS 反色配置 ==========
// IPS 面板出厂默认像素极性与普通 TFT 相反
// 必须开启反色，否则颜色全反（白底变黑底）
#define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF  // 如果颜色异常，尝试切换

// ========== 颜色顺序 ==========
// ST7789 默认 RGB 顺序，如果红蓝色 swapped 则改为 TFT_BGR
#define TFT_RGB_ORDER TFT_RGB
// #define TFT_RGB_ORDER TFT_BGR

// ========== SPI 频率 ==========
// 写屏频率：ST7789 最高支持 62.5MHz，40MHz 是安全值
#define SPI_FREQUENCY  40000000   // 40MHz（硬件 SPI 推荐）
// #define SPI_FREQUENCY  27000000  // 27MHz（保守值，排查问题时用）
// #define SPI_FREQUENCY  10000000  // 10MHz（最低速，仅调试）

// 读屏频率：用于 TFT 屏幕回读（如屏幕截图）
#define SPI_READ_FREQUENCY  8000000   // 8MHz

// ========== 字体加载 ==========
// 根据需求启用字体，注释掉不需要的以节省 Flash
#define LOAD_GLCD     // Font 1: Adafruit 8px 默认字体（~1820 bytes）
#define LOAD_FONT2    // Font 2: 16px 小字体（~3534 bytes）
#define LOAD_FONT4    // Font 4: 26px 中等字体（~5848 bytes）
#define LOAD_FONT6    // Font 6: 48px 大号数字字体（~2666 bytes）
#define LOAD_FONT7    // Font 7: 48px 7段数码管字体（~2438 bytes）
#define LOAD_FONT8    // Font 8: 75px 超大号字体（~3256 bytes）
// #define LOAD_FONT8N   // Font 8N: Font 8 窄版变体
#define LOAD_GFXFF    // FreeFonts: Adafruit_GFX 48 种免费字体

// 平滑字体（抗锯齿），用于 .vlw 字体文件渲染
#define SMOOTH_FONT

// ========== 事务支持 ==========
// 必须开启！允许与其他 SPI 设备（如字库芯片）共享总线
#define SUPPORT_TRANSACTIONS

// ========== 调试（开发时开启）==========
// #define TFT_DEBUG   // 在串口输出 TFT_eSPI 初始化调试信息
