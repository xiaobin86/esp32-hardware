import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, Circle, FancyArrowPatch
import numpy as np

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

fig, ax = plt.subplots(1, 1, figsize=(14, 10))
ax.set_xlim(0, 14)
ax.set_ylim(0, 10)
ax.axis('off')

# 标题
ax.text(7, 9.5, 'INMP441 I2S Microphone Wiring Diagram', fontsize=18, ha='center', fontweight='bold')
ax.text(7, 9.1, 'ESP32-S3-DevKitC-1  -  Full Duplex I2S (Shared with MAX98357A)', fontsize=12, ha='center', color='#666')

# ── ESP32-S3 主板（J3 右侧）───────────────────────
esp_x, esp_y = 1.5, 1.5
esp_w, esp_h = 3.5, 6

esp = FancyBboxPatch((esp_x, esp_y), esp_w, esp_h, boxstyle="round,pad=0.05",
                      facecolor='#2c3e50', edgecolor='#1a252f', linewidth=2)
ax.add_patch(esp)
ax.text(esp_x + esp_w/2, esp_y + esp_h - 0.3, 'ESP32-S3\nDevKitC-1', fontsize=11, ha='center',
        color='white', fontweight='bold')
ax.text(esp_x + esp_w/2, esp_y + esp_h - 0.8, 'J3 Right Side', fontsize=9, ha='center', color='#aaa')

# J3 引脚（从上到下）
j3_pins = [
    (1, 'GND',  '#e74c3c'),
    (2, 'TX/G43', '#95a5a6'),
    (3, 'RX/G44', '#95a5a6'),
    (4, 'GPIO1',  '#3498db'),   # I2S DOUT (to MAX98357A)
    (5, 'GPIO2',  '#e67e22'),   # I2S DIN (from INMP441) ★
    (6, 'GPIO42', '#95a5a6'),
    (7, 'GPIO41', '#95a5a6'),
    (8, 'GPIO40', '#95a5a6'),
    (9, 'GPIO39', '#95a5a6'),
    (10, 'GPIO38', '#95a5a6'),
    (11, 'GPIO37', '#95a5a6'),
    (12, 'GPIO36', '#95a5a6'),
    (13, 'GPIO35', '#95a5a6'),
    (14, 'GPIO0',  '#95a5a6'),
    (15, 'GPIO45', '#95a5a6'),
    (16, 'GPIO48', '#95a5a6'),
    (17, 'GPIO47', '#27ae60'),   # I2S BCLK (shared) ★
    (18, 'GPIO21', '#27ae60'),   # I2S WS (shared) ★
    (19, 'GPIO20', '#95a5a6'),
    (20, 'GPIO19', '#95a5a6'),
    (21, 'GND',    '#e74c3c'),
    (22, 'GND',    '#e74c3c'),
]

pin_h = esp_h / 23
for i, (num, name, color) in enumerate(j3_pins):
    py = esp_y + esp_h - 0.9 - (i + 1) * pin_h
    # 引脚编号
    ax.add_patch(Circle((esp_x + 0.25, py), 0.12, facecolor=color, edgecolor='white', linewidth=0.5))
    ax.text(esp_x + 0.25, py, str(num), fontsize=5, ha='center', va='center', color='white', fontweight='bold')
    # 引脚名称
    ax.text(esp_x + 0.55, py, name, fontsize=6, va='center', color='white')

# ── INMP441 模块（两排6针）────────────────────────
mic_x, mic_y = 9, 4
mic_w, mic_h = 3.5, 3

mic = FancyBboxPatch((mic_x, mic_y), mic_w, mic_h, boxstyle="round,pad=0.05",
                      facecolor='#8e44ad', edgecolor='#6c3483', linewidth=2)
ax.add_patch(mic)
ax.text(mic_x + mic_w/2, mic_y + mic_h - 0.3, 'INMP441', fontsize=12, ha='center',
        color='white', fontweight='bold')
ax.text(mic_x + mic_w/2, mic_y + mic_h - 0.7, 'I2S MEMS Mic', fontsize=9, ha='center', color='#ddd')

# 两排引脚
mic_pins_top = ['SD', 'VDD', 'GND']
mic_pins_bot = ['L/R', 'WS', 'SCK']

for i, name in enumerate(mic_pins_top):
    px = mic_x + 0.5 + i * 1.0
    py = mic_y + mic_h - 1.2
    ax.add_patch(Circle((px, py), 0.15, facecolor='#f39c12', edgecolor='white', linewidth=1))
    ax.text(px, py, name, fontsize=7, ha='center', va='center', color='white', fontweight='bold')
    ax.text(px, py + 0.4, name, fontsize=7, ha='center', va='center', color='white')

for i, name in enumerate(mic_pins_bot):
    px = mic_x + 0.5 + i * 1.0
    py = mic_y + 0.8
    ax.add_patch(Circle((px, py), 0.15, facecolor='#f39c12', edgecolor='white', linewidth=1))
    ax.text(px, py, name, fontsize=7, ha='center', va='center', color='white', fontweight='bold')
    ax.text(px, py - 0.4, name, fontsize=7, ha='center', va='center', color='white')

# 在模块上标注物理布局
ax.text(mic_x + mic_w/2, mic_y + mic_h/2, 'Top: SD  VDD  GND\nBot: L/R  WS  SCK',
        fontsize=8, ha='center', va='center', color='white', alpha=0.7)

# ── 接线连线 ──────────────────────────────────────

# 颜色定义
clk_color   = '#27ae60'  # 绿色：共享时钟
data_color  = '#e67e22'  # 橙色：数据输入
pwr_color   = '#e74c3c'  # 红色：电源

# GPIO2 (J3-5) → SD (上排左)
ax.annotate('', xy=(mic_x + 0.5, mic_y + mic_h - 1.2), xytext=(esp_x + esp_w, esp_y + esp_h - 0.9 - 5 * pin_h),
            arrowprops=dict(arrowstyle='->', color=data_color, lw=2))
ax.text(6.5, 6.8, 'GPIO2 → SD\n(Data In)', fontsize=8, ha='center', color=data_color, fontweight='bold',
        bbox=dict(boxstyle='round,pad=0.3', facecolor='white', edgecolor=data_color, alpha=0.9))

# GPIO47 (J3-17) → SCK (下排右)
ax.annotate('', xy=(mic_x + 2.5, mic_y + 0.8), xytext=(esp_x + esp_w, esp_y + esp_h - 0.9 - 17 * pin_h),
            arrowprops=dict(arrowstyle='->', color=clk_color, lw=2))
ax.text(6.5, 3.2, 'GPIO47 → SCK\n(BCLK shared)', fontsize=8, ha='center', color=clk_color, fontweight='bold',
        bbox=dict(boxstyle='round,pad=0.3', facecolor='white', edgecolor=clk_color, alpha=0.9))

# GPIO21 (J3-18) → WS (下排中)
ax.annotate('', xy=(mic_x + 1.5, mic_y + 0.8), xytext=(esp_x + esp_w, esp_y + esp_h - 0.9 - 18 * pin_h),
            arrowprops=dict(arrowstyle='->', color=clk_color, lw=2))
ax.text(6.5, 2.5, 'GPIO21 → WS\n(LRC shared)', fontsize=8, ha='center', color=clk_color, fontweight='bold',
        bbox=dict(boxstyle='round,pad=0.3', facecolor='white', edgecolor=clk_color, alpha=0.9))

# GND → GND
ax.annotate('', xy=(mic_x + 1.5, mic_y + mic_h - 1.2), xytext=(esp_x + esp_w, esp_y + esp_h - 0.9 - 21 * pin_h),
            arrowprops=dict(arrowstyle='->', color=pwr_color, lw=1.5, linestyle='--'))
ax.text(7.8, 5.8, 'GND', fontsize=8, ha='center', color=pwr_color)

# 3.3V → VDD (从 ESP32 的 3V3 引脚，用 J1 的 3V3)
ax.plot([esp_x + esp_w/2, esp_x + esp_w/2, mic_x + 0.5],
        [esp_y + esp_h, esp_y + esp_h + 0.3, mic_y + mic_h - 1.2],
        color='#3498db', lw=1.5, linestyle='--')
ax.text(5.5, 8.5, '3.3V → VDD', fontsize=8, ha='center', color='#3498db')

# L/R → GND (在模块旁标注)
ax.text(mic_x - 0.8, mic_y + 0.8, 'L/R → GND\n(Left Ch)', fontsize=7, ha='center', color='#666',
        bbox=dict(boxstyle='round,pad=0.2', facecolor='#f0f0f0', edgecolor='#999'))

# ── 图例 ─────────────────────────────────────────
legend_x, legend_y = 1, 0.3
ax.add_patch(FancyBboxPatch((legend_x, legend_y), 5.5, 1.8, boxstyle="round,pad=0.05",
                             facecolor='#f8f9fa', edgecolor='#dee2e6', linewidth=1))
ax.text(legend_x + 0.2, legend_y + 1.5, 'Legend:', fontsize=9, fontweight='bold')

ax.plot([legend_x + 0.3, legend_x + 0.8], [legend_y + 1.1, legend_y + 1.1], color=clk_color, lw=2)
ax.text(legend_x + 1.0, legend_y + 1.1, 'Shared Clock (BCLK/WS)', fontsize=8, va='center')

ax.plot([legend_x + 0.3, legend_x + 0.8], [legend_y + 0.7, legend_y + 0.7], color=data_color, lw=2)
ax.text(legend_x + 1.0, legend_y + 0.7, 'Data In (Microphone → ESP32)', fontsize=8, va='center')

ax.plot([legend_x + 0.3, legend_x + 0.8], [legend_y + 0.3, legend_y + 0.3], color=pwr_color, lw=1.5, linestyle='--')
ax.text(legend_x + 1.0, legend_y + 0.3, 'Power/GND', fontsize=8, va='center')

# ── 关键说明 ─────────────────────────────────────
note_x, note_y = 7, 0.3
ax.add_patch(FancyBboxPatch((note_x, note_y), 6, 1.8, boxstyle="round,pad=0.05",
                             facecolor='#fff3cd', edgecolor='#ffc107', linewidth=1))
ax.text(note_x + 0.2, note_y + 1.5, 'Key Points:', fontsize=9, fontweight='bold', color='#856404')
ax.text(note_x + 0.2, note_y + 1.1, '• GPIO47/GPIO21 shared with MAX98357A (same I2S bus)', fontsize=8, color='#856404')
ax.text(note_x + 0.2, note_y + 0.7, '• GPIO2 is dedicated data input from microphone', fontsize=8, color='#856404')
ax.text(note_x + 0.2, note_y + 0.3, '• ESP32-S3 I2S full-duplex: TX to amp + RX from mic simultaneously', fontsize=8, color='#856404')

plt.tight_layout()
plt.savefig('INMP441_wiring_diagram.png', dpi=150, bbox_inches='tight', facecolor='white')
print("接线图已保存为 INMP441_wiring_diagram.png")
