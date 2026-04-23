#include "driver/i2s.h"
#include <math.h>

#define I2S_BCLK  47
#define I2S_LRC   21
#define I2S_DIN    1
#define I2S_PORT  I2S_NUM_0
#define SAMPLE_RATE 44100
#define VOLUME  0.12f        // 0.0~1.0，调这里控制总音量

// ── ADSR 包络参数（单位：毫秒）─────────────────
#define ATTACK_MS   30       // 起音时间，越大越软
#define DECAY_MS    200      // 衰减到 sustain 的时间
#define SUSTAIN_LVL 0.5f     // 延音电平（0.0~1.0）
#define RELEASE_MS  400      // 释音时间，尾音长度
#define NOTE_MS     1200     // 每个音符总时长

// ── C 大调音阶频率（Hz）───────────────────────
const float NOTES[] = {
  261.63f,  // C4
  293.66f,  // D4
  329.63f,  // E4
  349.23f,  // F4
  392.00f,  // G4
  440.00f,  // A4
  493.88f,  // B4
  523.25f,  // C5
};
const int NOTE_COUNT = 8;

// ── I2S 初始化 ────────────────────────────────
void i2s_init() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 128,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);

  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_LRC,
    .data_out_num = I2S_DIN,
    .data_in_num  = I2S_PIN_NO_CHANGE,
  };
  i2s_set_pin(I2S_PORT, &pins);
}

// ── ADSR 包络计算 ─────────────────────────────
float adsr(int sample, int total_samples) {
  int atk  = (int)(ATTACK_MS  * SAMPLE_RATE / 1000);
  int dec  = (int)(DECAY_MS   * SAMPLE_RATE / 1000);
  int rel  = (int)(RELEASE_MS * SAMPLE_RATE / 1000);
  int sus_end = total_samples - rel;

  if (sample < atk) {
    return (float)sample / atk;                          // 线性起音
  } else if (sample < atk + dec) {
    float t = (float)(sample - atk) / dec;
    return 1.0f - t * (1.0f - SUSTAIN_LVL);             // 衰减到 sustain
  } else if (sample < sus_end) {
    return SUSTAIN_LVL;                                  // 延音平台
  } else {
    float t = (float)(sample - sus_end) / rel;
    return SUSTAIN_LVL * (1.0f - t);                    // 释音淡出
  }
}

// ── 合成一个音符并输出 ────────────────────────
// 基频 + 谐波混合模拟钢琴泛音
// 谐波比例：1:0.5:0.25:0.12（自然泛音列）
void play_note(float freq, int duration_ms) {
  const int total = duration_ms * SAMPLE_RATE / 1000;
  const int BUF = 128;
  int16_t buf[BUF * 2];  // 立体声

  float phase1 = 0, phase2 = 0, phase3 = 0, phase4 = 0;
  float inc1 = 2.0f * M_PI * freq           / SAMPLE_RATE;
  float inc2 = 2.0f * M_PI * freq * 2.0f    / SAMPLE_RATE;  // 2 次谐波
  float inc3 = 2.0f * M_PI * freq * 3.0f    / SAMPLE_RATE;  // 3 次谐波
  float inc4 = 2.0f * M_PI * freq * 4.0f    / SAMPLE_RATE;  // 4 次谐波

  for (int s = 0; s < total; s += BUF) {
    int chunk = min(BUF, total - s);
    for (int i = 0; i < chunk; i++) {
      float env = adsr(s + i, total);

      // 泛音叠加（归一化权重总和 = 1.87，故除以 1.87）
      float sample = (sinf(phase1)        // 基频     权重 1.00
                    + sinf(phase2) * 0.50f // 2次谐波  权重 0.50
                    + sinf(phase3) * 0.25f // 3次谐波  权重 0.25
                    + sinf(phase4) * 0.12f // 4次谐波  权重 0.12
                    ) / 1.87f;

      int16_t out = (int16_t)(sample * env * VOLUME * 32767.0f);
      buf[i * 2]     = out;  // 左声道
      buf[i * 2 + 1] = out;  // 右声道

      phase1 += inc1; if (phase1 > 2*M_PI) phase1 -= 2*M_PI;
      phase2 += inc2; if (phase2 > 2*M_PI) phase2 -= 2*M_PI;
      phase3 += inc3; if (phase3 > 2*M_PI) phase3 -= 2*M_PI;
      phase4 += inc4; if (phase4 > 2*M_PI) phase4 -= 2*M_PI;
    }
    size_t written = 0;
    i2s_write(I2S_PORT, buf, chunk * 4, &written, portMAX_DELAY);
  }
}

void setup() {
  Serial.begin(115200);
  i2s_init();
  Serial.println("开始播放音阶...");
}

void loop() {
  // 上行音阶
  for (int i = 0; i < NOTE_COUNT; i++) {
    play_note(NOTES[i], NOTE_MS);
    delay(50);  // 音符间短暂静音，分隔感更清晰
  }
  // 下行音阶
  for (int i = NOTE_COUNT - 2; i > 0; i--) {
    play_note(NOTES[i], NOTE_MS);
    delay(50);
  }
  delay(800);  // 循环间停顿
}