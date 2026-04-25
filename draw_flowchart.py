import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
import numpy as np

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

fig, ax = plt.subplots(1, 1, figsize=(18, 14))
ax.set_xlim(0, 18)
ax.set_ylim(0, 14)
ax.axis('off')

# 颜色定义
color_title = '#1e40af'
color_primary = '#3b82f6'
color_success = '#10b981'
color_warning = '#f59e0b'
color_danger = '#ef4444'
color_bg_light = '#dbeafe'
color_bg_green = '#d1fae5'
color_bg_red = '#fee2e2'
color_bg_gray = '#f3f4f6'
color_text = '#374151'
color_text_light = '#6b7280'

# ========== 标题 ==========
ax.text(9, 13.5, '硬件SPI共享总线：读取字库 → 显示汉字 完整流程', 
        fontsize=20, fontweight='bold', ha='center', color=color_title)
ax.text(9, 13.0, 'ESP32-S3 FSPI总线分时复用  |  TFT_CS=GPIO10  |  FONT_CS=GPIO9', 
        fontsize=12, ha='center', color=color_text_light)

# ========== 阶段一：字库读取 ==========
ax.text(0.5, 12.3, '▶ 阶段一：字库读取 (fontRead)', fontsize=14, fontweight='bold', color=color_title)
ax.plot([0.5, 17.5], [12.0, 12.0], 'k--', alpha=0.3, linewidth=1)

# 步骤框
steps_phase1 = [
    (0.5, 10.8, 2.8, 0.9, '① 防御性拉高 TFT_CS\n   digitalWrite(TFT_CS, HIGH)', color_bg_light, color_primary),
    (3.8, 10.8, 2.8, 0.9, '② SPI.beginTransaction()\n   切换为字库参数 MODE0, 8MHz', color_bg_light, color_primary),
    (7.1, 10.8, 2.8, 0.9, '③ 拉低 FONT_CS\n   digitalWrite(FONT_CS, LOW)\n   ✅ 字库被选中', color_bg_green, color_success),
    (10.4, 10.8, 2.8, 0.9, '④ 发送 READ 指令\n   SPI.transfer(0x03)\n   + 3字节地址', color_bg_light, color_primary),
    (13.7, 10.8, 3.3, 0.9, '⑤ 读取 32 字节字模\n   for(i=0; i<32; i++)\n   fontBuf[i]=SPI.transfer(0x00)', color_bg_green, color_success),
]

for i, (x, y, w, h, text, bg_color, border_color) in enumerate(steps_phase1):
    box = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.05", 
                         facecolor=bg_color, edgecolor=border_color, linewidth=2)
    ax.add_patch(box)
    ax.text(x + w/2, y + h/2, text, fontsize=9, ha='center', va='center', color=color_text)
    if i < len(steps_phase1) - 1:
        ax.annotate('', xy=(steps_phase1[i+1][0], y + h/2), xytext=(x + w, y + h/2),
                   arrowprops=dict(arrowstyle='->', color=color_primary, lw=2))

# ========== CS 信号时序图 ==========
ax.text(0.5, 9.8, '📊 CS 信号状态变化', fontsize=13, fontweight='bold', color=color_title)

# 时间轴
ax.plot([2, 16], [9.2, 9.2], 'k-', linewidth=2)
ax.text(1.5, 9.2, '时间 →', fontsize=10, ha='center', va='center')

# TFT_CS 信号
ax.text(0.5, 8.7, 'TFT_CS', fontsize=11, fontweight='bold', color=color_danger)
tft_y = 8.4
# HIGH -> HIGH -> HIGH -> LOW -> LOW
ax.plot([2, 8], [tft_y, tft_y], color=color_danger, linewidth=3)
ax.plot([8, 8], [tft_y, tft_y+0.3], color=color_danger, linewidth=3)
ax.plot([8, 16], [tft_y+0.3, tft_y+0.3], color=color_danger, linewidth=3)
ax.text(5, tft_y+0.15, 'HIGH (空闲)', fontsize=9, ha='center', color=color_danger)
ax.text(12, tft_y+0.45, 'LOW (选中屏幕)', fontsize=9, ha='center', color=color_danger)

# FONT_CS 信号
ax.text(0.5, 7.7, 'FONT_CS', fontsize=11, fontweight='bold', color=color_success)
font_y = 7.4
# HIGH -> LOW -> LOW -> HIGH -> HIGH
ax.plot([2, 5.5], [font_y, font_y], color=color_success, linewidth=3)
ax.plot([5.5, 5.5], [font_y, font_y+0.3], color=color_success, linewidth=3)
ax.plot([5.5, 10], [font_y+0.3, font_y+0.3], color=color_success, linewidth=3)
ax.plot([10, 10], [font_y+0.3, font_y], color=color_success, linewidth=3)
ax.plot([10, 16], [font_y, font_y], color=color_success, linewidth=3)
ax.text(3.75, font_y+0.15, 'HIGH (空闲)', fontsize=9, ha='center', color=color_success)
ax.text(7.75, font_y+0.45, 'LOW (选中字库)', fontsize=9, ha='center', color=color_success)
ax.text(13, font_y+0.15, 'HIGH (空闲)', fontsize=9, ha='center', color=color_success)

# 阶段标注
ax.axvline(x=8, color='gray', linestyle='--', alpha=0.5)
ax.text(5, 6.8, '🔵 字库读取阶段', fontsize=11, ha='center', color=color_primary, fontweight='bold')
ax.text(12, 6.8, '🔴 屏幕显示阶段', fontsize=11, ha='center', color=color_danger, fontweight='bold')

# ========== 阶段二：解码 ==========
ax.text(0.5, 6.2, '▶ 阶段二：解码点阵 → RGB565', fontsize=14, fontweight='bold', color=color_title)
ax.plot([0.5, 17.5], [5.9, 5.9], 'k--', alpha=0.3, linewidth=1)

decode_box = FancyBboxPatch((0.5, 4.8), 8, 0.9, boxstyle="round,pad=0.05",
                            facecolor=color_bg_gray, edgecolor=color_primary, linewidth=2)
ax.add_patch(decode_box)
ax.text(4.5, 5.25, 'for(row=0; row<16; row++) {\n  b0=fontBuf[row*2]; b1=fontBuf[row*2+1];\n  for(col=0; col<8; col++) { pixBuf[...] = (b0 & (0x80>>col)) ? fg : bg; }\n}',
        fontsize=9, ha='center', va='center', color=color_text, family='monospace')

ax.text(9.5, 5.25, '将 1bit 点阵解码为 RGB565 像素缓冲\nfontBuf[32] → pixBuf[256] (16×16)', 
        fontsize=11, ha='left', va='center', color=color_text_light)

# ========== 阶段三：屏幕显示 ==========
ax.text(0.5, 4.2, '▶ 阶段三：屏幕显示 (drawRGBBitmap)', fontsize=14, fontweight='bold', color=color_title)
ax.plot([0.5, 17.5], [3.9, 3.9], 'k--', alpha=0.3, linewidth=1)

steps_phase3 = [
    (0.5, 2.7, 3.5, 0.9, '① 调用 drawRGBBitmap()\n   内部自动执行 startWrite()\n   → csLow() 拉低 TFT_CS', color_bg_red, color_danger),
    (4.5, 2.7, 3.5, 0.9, '② 设置窗口坐标\n   setAddrWindow(x, y, 16, 16)\n   准备写入 16×16 像素区域', color_bg_light, color_primary),
    (8.5, 2.7, 3.5, 0.9, '③ 批量发送像素数据\n   SPI 发送 pixBuf[256]\n   16×16 RGB565 像素', color_bg_green, color_success),
    (12.5, 2.7, 3.5, 0.9, '④ endWrite() 自动执行\n   csHigh() 拉高 TFT_CS\n   ✅ 屏幕显示完成', color_bg_green, color_success),
]

for i, (x, y, w, h, text, bg_color, border_color) in enumerate(steps_phase3):
    box = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.05",
                         facecolor=bg_color, edgecolor=border_color, linewidth=2)
    ax.add_patch(box)
    ax.text(x + w/2, y + h/2, text, fontsize=9, ha='center', va='center', color=color_text)
    if i < len(steps_phase3) - 1:
        ax.annotate('', xy=(steps_phase3[i+1][0], y + h/2), xytext=(x + w, y + h/2),
                   arrowprops=dict(arrowstyle='->', color=color_danger, lw=2))

# ========== 关键代码证据 ==========
code_box = FancyBboxPatch((0.5, 0.3), 17, 1.8, boxstyle="round,pad=0.05",
                          facecolor='#1e293b', edgecolor='#374151', linewidth=2)
ax.add_patch(code_box)

code_text = '''📋 关键代码证据 (Evidence)

// ===== 字库读取：手动控制 CS =====                                      // ===== 屏幕显示：库自动管理 CS (Adafruit_SPITFT.cpp) =====
bool fontRead(uint32_t addr) {                                             void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h) {
  digitalWrite(TFT_CS, HIGH);          // ⬆️ 确保屏幕未选中              startWrite();           // ← 内部调用 csLow()  ⬇️ 自动拉低 TFT_CS
  SPI.beginTransaction(SPISettings(8MHz, MSBFIRST, SPI_MODE0));            setAddrWindow(x, y, w, h);
  digitalWrite(FONT_CS, LOW);           // ⬇️ 选中字库                    // ... 通过 SPI 写入像素数据 ...
  SPI.transfer(0x03); ... // 发送 READ + 地址                            endWrite();             // ← 内部调用 csHigh() ⬆️ 自动拉高 TFT_CS
  for (int i=0; i<32; i++) fontBuf[i] = SPI.transfer(0x00);  // 读字模  }
  digitalWrite(FONT_CS, HIGH);         // ⬆️ 释放字库
  SPI.endTransaction();
}'''

ax.text(1.0, 1.9, code_text, fontsize=8, ha='left', va='top', color='#22c55e', family='monospace')

plt.tight_layout()
plt.savefig('D:\\work\\esp32-demo\\hardware_spi_flowchart.png', dpi=150, bbox_inches='tight', 
            facecolor='white', edgecolor='none')
plt.show()
