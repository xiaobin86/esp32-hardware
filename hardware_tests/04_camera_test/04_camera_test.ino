/**
 * ============================================
 * 04_camera_test.ino
 * 四足机器马 - OV2640 摄像头测试
 * ============================================
 * 
 * 功能说明：
 *   - 初始化 OV2640 摄像头
 *   - 通过串口输出图像帧信息
 *   - 开启 WiFi HTTP 服务器，浏览器查看实时画面
 * 
 * 接线（OV2640 DVP 24Pin -> ESP32-S3）：
 *   摄像头 3.3V -> 3.3V
 *   摄像头 GND  -> GND
 *   摄像头 D0   -> GPIO 11
 *   摄像头 D1   -> GPIO 13
 *   摄像头 D2   -> GPIO 15
 *   摄像头 D3   -> GPIO 16
 *   摄像头 D4   -> GPIO 17
 *   摄像头 D5   -> GPIO 18
 *   摄像头 D6   -> GPIO 19
 *   摄像头 D7   -> GPIO 20
 *   摄像头 PCLK  -> GPIO 12
 *   摄像头 VSYNC -> GPIO 14
 *   摄像头 HREF  -> GPIO 21
 *   摄像头 XCLK  -> GPIO 10
 *   摄像头 SDA   -> GPIO 8
 *   摄像头 SCL   -> GPIO 9
 *   摄像头 PWDN  -> GPIO 7 (或接 GND)
 *   摄像头 RESET -> GPIO 6 (或接 3.3V)
 * 
 * 需要的库：
 *   - esp32-camera (ESP32 官方摄像头库)
 *     安装方式：Arduino IDE -> 项目 -> 加载库 -> 管理库 -> 搜索 "esp32-camera"
 *     或者从 https://github.com/espressif/esp32-camera 下载
 * 
 * 使用方式：
 *   1. 上传代码到 ESP32-S3
 *   2. 打开串口监视器，查看启动信息
 *   3. 等待 WiFi 连接成功，记录 IP 地址
 *   4. 手机/电脑连接同一 WiFi，浏览器访问 http://<IP地址>/
 *   5. 点击 "查看摄像头画面" 按钮
 */

#include <WiFi.h>
#include "esp_camera.h"

// ============================================
// WiFi 配置（修改为你的网络）
// ============================================
const char* ssid = "YOUR_WIFI_SSID";      // <-- 修改为你的 WiFi 名称
const char* password = "YOUR_WIFI_PASSWORD"; // <-- 修改为你的 WiFi 密码

// 如果不使用 WiFi，设为 true（仅通过串口输出调试信息）
#define NO_WIFI_MODE false

// ============================================
// 摄像头引脚定义（ESP32-S3 DVP 接口）
// ============================================
#define CAM_PIN_PWDN    7
#define CAM_PIN_RESET   6
#define CAM_PIN_XCLK    10
#define CAM_PIN_SIOD    8    // SDA
#define CAM_PIN_SIOC    9    // SCL

#define CAM_PIN_D7      20
#define CAM_PIN_D6      19
#define CAM_PIN_D5      18
#define CAM_PIN_D4      17
#define CAM_PIN_D3      16
#define CAM_PIN_D2      15
#define CAM_PIN_D1      13
#define CAM_PIN_D0      11

#define CAM_PIN_VSYNC   14
#define CAM_PIN_HREF    21
#define CAM_PIN_PCLK    12

// ============================================
// HTTP 服务器端口
// ============================================
WiFiServer server(80);

// ============================================
// HTML 页面内容
// ============================================
const char* HTML_PAGE = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>四足机器马 - 摄像头画面</title>
    <style>
        body { font-family: Arial; text-align: center; background: #1a1a1a; color: #fff; margin: 0; padding: 20px; }
        h1 { color: #4CAF50; }
        .container { max-width: 640px; margin: 0 auto; }
        img { max-width: 100%; border: 2px solid #4CAF50; border-radius: 8px; }
        .info { background: #333; padding: 10px; border-radius: 5px; margin: 10px 0; }
        button { background: #4CAF50; color: white; border: none; padding: 15px 30px; 
                 font-size: 16px; cursor: pointer; border-radius: 5px; margin: 5px; }
        button:hover { background: #45a049; }
        .status { color: #ff9800; }
    </style>
</head>
<body>
    <div class="container">
        <h1>四足机器马 摄像头</h1>
        <div class="info">
            <p>状态: <span class="status" id="status">等待连接...</span></p>
            <p>分辨率: <span id="resolution">-</span></p>
            <p>帧率: <span id="fps">-</span> FPS</p>
        </div>
        <img id="cam" src="" alt="摄像头画面" style="display:none;">
        <br><br>
        <button onclick="startStream()">开始视频流</button>
        <button onclick="capture()">拍照</button>
        <button onclick="stopStream()">停止</button>
    </div>
    <script>
        let streaming = false;
        let frameCount = 0;
        let lastTime = Date.now();
        
        function startStream() {
            streaming = true;
            document.getElementById('cam').style.display = 'block';
            document.getElementById('cam').src = '/stream';
            document.getElementById('status').textContent = '视频流中...';
            updateFPS();
        }
        
        function stopStream() {
            streaming = false;
            document.getElementById('cam').src = '';
            document.getElementById('cam').style.display = 'none';
            document.getElementById('status').textContent = '已停止';
        }
        
        function capture() {
            window.open('/capture', '_blank');
        }
        
        function updateFPS() {
            if (!streaming) return;
            const now = Date.now();
            const elapsed = (now - lastTime) / 1000;
            if (elapsed >= 1) {
                document.getElementById('fps').textContent = frameCount;
                frameCount = 0;
                lastTime = now;
            }
            frameCount++;
            requestAnimationFrame(updateFPS);
        }
        
        // 获取摄像头信息
        fetch('/info').then(r => r.json()).then(data => {
            document.getElementById('resolution').textContent = 
                data.width + 'x' + data.height;
        });
    </script>
</body>
</html>
)rawliteral";

// ============================================
// 摄像头配置
// ============================================
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,
    
    .xclk_freq_hz = 20000000,     // XCLK 20MHz
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    
    .pixel_format = PIXFORMAT_JPEG,  // JPEG 格式（适合网络传输）
    .frame_size = FRAMESIZE_QVGA,    // 320x240（平衡性能与画质）
    .jpeg_quality = 12,              // JPEG 质量 (0-63, 越低越好)
    .fb_count = 2,                   // 帧缓冲数量
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port = -1  // 使用内部 SCCB 驱动
};

// ============================================
// 初始化
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n============================================");
  Serial.println("   四足机器马 - OV2640 摄像头测试");
  Serial.println("============================================");
  
  // 初始化摄像头
  Serial.println("\n[初始化] 正在启动 OV2640 摄像头...");
  
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("[错误] 摄像头初始化失败! 错误码: 0x%x\n", err);
    Serial.println("       常见原因：");
    Serial.println("       1. 引脚接线错误");
    Serial.println("       2. 摄像头模块损坏");
    Serial.println("       3. PSRAM 未启用（需要在 Arduino IDE 中配置）");
    while (1) { delay(100); }
  }
  
  Serial.println("[成功] 摄像头初始化完成！");
  
  // 获取摄像头信息
  sensor_t *s = esp_camera_sensor_get();
  Serial.printf("\n[信息] 摄像头型号: %s\n", s->id.PID == OV2640_PID ? "OV2640" : "未知");
  Serial.printf("       分辨率: %dx%d\n", camera_config.frame_size == FRAMESIZE_QVGA ? 320 : 640,
                camera_config.frame_size == FRAMESIZE_QVGA ? 240 : 480);
  
  // 设置摄像头参数
  s->set_brightness(s, 0);     // 亮度
  s->set_contrast(s, 0);       // 对比度
  s->set_saturation(s, 0);     // 饱和度
  s->set_special_effect(s, 0); // 特效
  s->set_whitebal(s, 1);       // 自动白平衡
  s->set_awb_gain(s, 1);       // 自动白平衡增益
  s->set_exposure_ctrl(s, 1);  // 自动曝光
  
  // 测试采集一帧
  Serial.println("\n[测试] 采集一帧图像...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[错误] 图像采集失败！");
  } else {
    Serial.printf("[成功] 图像采集成功! 大小: %d bytes\n", fb->len);
    esp_camera_fb_return(fb);
  }
  
#if !NO_WIFI_MODE
  // 连接 WiFi
  Serial.println("\n[WiFi] 正在连接...");
  WiFi.begin(ssid, password);
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[成功] WiFi 已连接！");
    Serial.printf("       IP 地址: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("       子网掩码: %s\n", WiFi.subnetMask().toString().c_str());
    
    server.begin();
    Serial.println("[HTTP] 服务器已启动，端口 80");
    Serial.println("\n============================================");
    Serial.println("   使用方式:");
    Serial.print("   浏览器访问: http://");
    Serial.print(WiFi.localIP().toString().c_str());
    Serial.println("/");
    Serial.println("============================================\n");
  } else {
    Serial.println("\n[警告] WiFi 连接失败！进入无 WiFi 模式。");
    Serial.println("       请检查 WiFi 名称和密码是否正确。");
  }
#else
  Serial.println("\n[模式] 无 WiFi 模式，仅通过串口输出调试信息。");
#endif
}

// ============================================
// 主循环
// ============================================
void loop() {
#if !NO_WIFI_MODE
  // 处理 HTTP 请求
  if (WiFi.status() == WL_CONNECTED) {
    handleHttpClient();
  }
#else
  // 无 WiFi 模式：定期采集并输出帧信息
  static unsigned long lastCapture = 0;
  if (millis() - lastCapture >= 2000) {
    lastCapture = millis();
    testCapture();
  }
#endif
}

// ============================================
// 无 WiFi 模式测试采集
// ============================================
void testCapture() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[错误] 图像采集失败");
    return;
  }
  
  Serial.printf("[采集] 帧大小: %d bytes, 分辨率: %dx%d\n", 
                fb->len, fb->width, fb->height);
  
  // 输出前 16 字节的十六进制（JPEG 头）
  Serial.print("       JPEG 头部: ");
  for (int i = 0; i < min(16, (int)fb->len); i++) {
    Serial.printf("%02X ", fb->buf[i]);
  }
  Serial.println();
  
  esp_camera_fb_return(fb);
}

// ============================================
// HTTP 请求处理
// ============================================
void handleHttpClient() {
  WiFiClient client = server.available();
  if (!client) return;
  
  unsigned long timeout = millis() + 3000;
  while (!client.available() && millis() < timeout) {
    delay(1);
  }
  
  if (!client.available()) {
    client.stop();
    return;
  }
  
  // 读取请求行
  String req = client.readStringUntil('\r');
  client.readStringUntil('\n');
  
  Serial.printf("[HTTP] 请求: %s\n", req.c_str());
  
  // 跳过请求头
  while (client.available()) {
    String line = client.readStringUntil('\r');
    client.readStringUntil('\n');
    if (line.length() <= 1) break;
  }
  
  // 路由处理
  if (req.indexOf("GET / ") >= 0 || req.indexOf("GET /index") >= 0) {
    sendHtmlPage(client);
  } else if (req.indexOf("GET /stream") >= 0) {
    sendStream(client);
  } else if (req.indexOf("GET /capture") >= 0) {
    sendCapture(client);
  } else if (req.indexOf("GET /info") >= 0) {
    sendInfo(client);
  } else {
    send404(client);
  }
  
  client.stop();
}

// ============================================
// 发送 HTML 页面
// ============================================
void sendHtmlPage(WiFiClient &client) {
  String html = HTML_PAGE;
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println(html);
}

// ============================================
// 发送视频流 (MJPEG)
// ============================================
void sendStream(WiFiClient &client) {
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n";
  response += "\r\n";
  client.print(response);
  
  int frameCount = 0;
  unsigned long startTime = millis();
  
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;
    
    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", 
                  fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    
    esp_camera_fb_return(fb);
    
    frameCount++;
    
    // 限制帧率约 15fps
    delay(60);
    
    // 每 5 秒输出一次统计
    if (millis() - startTime >= 5000) {
      float fps = frameCount * 1000.0 / (millis() - startTime);
      Serial.printf("[流] 已发送 %d 帧, 平均 %.1f FPS\n", frameCount, fps);
      frameCount = 0;
      startTime = millis();
    }
  }
}

// ============================================
// 发送单张图片
// ============================================
void sendCapture(WiFiClient &client) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    send500(client);
    return;
  }
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.printf("Content-Length: %d\r\n", fb->len);
  client.println("Connection: close");
  client.println();
  client.write(fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
  Serial.println("[HTTP] 发送单张图片");
}

// ============================================
// 发送摄像头信息 (JSON)
// ============================================
void sendInfo(WiFiClient &client) {
  sensor_t *s = esp_camera_sensor_get();
  
  String json = "{";
  json += "\"model\":\"OV2640\",";
  json += "\"width\":" + String(320) + ",";
  json += "\"height\":" + String(240) + ",";
  json += "\"format\":\"JPEG\"";
  json += "}";
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println(json);
}

// ============================================
// 发送 404
// ============================================
void send404(WiFiClient &client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("404 Not Found");
}

// ============================================
// 发送 500
// ============================================
void send500(WiFiClient &client) {
  client.println("HTTP/1.1 500 Internal Server Error");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("500 Error");
}
