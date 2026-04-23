from PIL import Image, ImageDraw, ImageFont
import os

# Create output directory
os.makedirs('outputs', exist_ok=True)

# Canvas size
W, H = 1200, 900
img = Image.new('RGB', (W, H), '#1a1a2e')
draw = ImageDraw.Draw(img)

# Colors
C_BOARD = '#16213e'
C_BOARD_BORDER = '#0f3460'
C_GY91 = '#1a1a2e'
C_GY91_BORDER = '#e94560'
C_PIN = '#533483'
C_PIN_TEXT = '#ffffff'
C_LINE = '#00d9ff'
C_LINE2 = '#ff6b6b'
C_LINE3 = '#51cf66'
C_LINE4 = '#ffd43b'
C_TITLE = '#ffffff'
C_LABEL = '#a8d8ea'
C_GND = '#ff8787'
C_VCC = '#69db7c'

# Try to load font, fallback to default
try:
    font_title = ImageFont.truetype("arial.ttf", 28)
    font_header = ImageFont.truetype("arial.ttf", 20)
    font_pin = ImageFont.truetype("arial.ttf", 14)
    font_small = ImageFont.truetype("arial.ttf", 12)
except:
    font_title = ImageFont.load_default()
    font_header = ImageFont.load_default()
    font_pin = ImageFont.load_default()
    font_small = ImageFont.load_default()

# Title
draw.text((W//2 - 280, 20), "GY-91 MPU9250+BMP280 接线图", fill=C_TITLE, font=font_title)
draw.text((W//2 - 200, 55), "ESP32-S3-DevKitC-1  ←→  GY-91 (I2C Mode)", fill=C_LABEL, font=font_header)

# ============= ESP32-S3 Board =============
board_x, board_y = 80, 120
board_w, board_h = 340, 640

# Board body
draw.rounded_rectangle([board_x, board_y, board_x + board_w, board_y + board_h], 
                        radius=15, fill=C_BOARD, outline=C_BOARD_BORDER, width=3)

# Board label
draw.text((board_x + 70, board_y + 15), "ESP32-S3-DevKitC-1", fill=C_TITLE, font=font_header)
draw.text((board_x + 120, board_y + 42), "I2C: GPIO3/4", fill=C_LABEL, font=font_small)

# USB port representation
draw.rounded_rectangle([board_x + 130, board_y + 60, board_x + 210, board_y + 85], 
                        radius=5, fill='#2d2d2d', outline='#555555', width=2)
draw.text((board_x + 145, board_y + 65), "USB-C", fill='#888888', font=font_small)

# ============= ESP32 Pin Headers =============
# Left header (J1)
j1_x = board_x + 20
j1_pins = [
    ("3.3V", C_VCC), ("GPIO5", C_PIN), ("GPIO6", C_PIN), ("GPIO7", C_PIN),
    ("GPIO8", C_PIN), ("GPIO9", C_PIN), ("GPIO10", C_PIN), ("GPIO11", C_PIN),
    ("GPIO12", C_PIN), ("GPIO13", C_PIN), ("GPIO14", C_PIN), ("GPIO15", C_PIN),
    ("GPIO16", C_PIN), ("GPIO17", C_PIN), ("GPIO18", C_PIN), ("GPIO19", C_PIN),
    ("GPIO20", C_PIN), ("GPIO21", C_PIN), ("GPIO26", C_PIN), ("GPIO47", C_PIN),
]

# Right header (J3)
j3_x = board_x + board_w - 20 - 100
j3_pins = [
    ("GPIO0", C_PIN), ("GPIO1", C_PIN), ("GPIO2", C_PIN), ("GPIO3", C_LINE),  # SDA
    ("GPIO4", C_LINE),  # SCL
    ("GPIO5", C_PIN), ("GPIO46", C_PIN), ("GPIO48", C_PIN),
    ("GPIO45", C_PIN), ("GPIO42", C_PIN), ("GPIO41", C_PIN), ("GPIO40", C_PIN),
    ("GPIO39", C_PIN), ("GPIO38", C_PIN), ("GPIO37", C_PIN), ("GPIO36", C_PIN),
    ("GPIO35", C_PIN), ("GPIO34", C_PIN), ("GPIO33", C_PIN), ("5V", C_VCC),
]

# Draw J1 pins
pin_y_start = board_y + 120
pin_h = 24
for i, (name, color) in enumerate(j1_pins):
    y = pin_y_start + i * pin_h
    # Pin rectangle
    draw.rounded_rectangle([j1_x, y, j1_x + 90, y + 18], radius=3, fill=color, outline='#444444', width=1)
    draw.text((j1_x + 5, y + 2), name, fill=C_PIN_TEXT, font=font_pin)

# Draw J3 pins
for i, (name, color) in enumerate(j3_pins):
    y = pin_y_start + i * pin_h
    draw.rounded_rectangle([j3_x, y, j3_x + 90, y + 18], radius=3, fill=color, outline='#444444', width=1)
    draw.text((j3_x + 5, y + 2), name, fill=C_PIN_TEXT, font=font_pin)

# Highlight I2C pins on board
sda_idx = 3  # GPIO3
scl_idx = 4  # GPIO4
sda_y = pin_y_start + sda_idx * pin_h + 9
scl_y = pin_y_start + scl_idx * pin_h + 9

# Draw highlight circles
draw.ellipse([j3_x - 8, sda_y - 8, j3_x + 8, sda_y + 8], outline=C_LINE, width=3)
draw.ellipse([j3_x - 8, scl_y - 8, j3_x + 8, scl_y + 8], outline=C_LINE, width=3)

# ============= GY-91 Module =============
gy91_x, gy91_y = 750, 200
gy91_w, gy91_h = 300, 420

# Module body
draw.rounded_rectangle([gy91_x, gy91_y, gy91_x + gy91_w, gy91_y + gy91_h], 
                        radius=12, fill=C_GY91, outline=C_GY91_BORDER, width=4)

# Module label
draw.text((gy91_x + 60, gy91_y + 15), "GY-91 10DOF Module", fill=C_GY91_BORDER, font=font_header)
draw.text((gy91_x + 70, gy91_y + 42), "MPU9250 + BMP280", fill=C_LABEL, font=font_small)

# Chip labels
draw.rounded_rectangle([gy91_x + 30, gy91_y + 70, gy91_x + 130, gy91_y + 100], 
                        radius=5, fill='#2d2d2d', outline='#e94560', width=2)
draw.text((gy91_x + 40, gy91_y + 77), "MPU9250", fill='#e94560', font=font_pin)

draw.rounded_rectangle([gy91_x + 160, gy91_y + 70, gy91_x + 260, gy91_y + 100], 
                        radius=5, fill='#2d2d2d', outline='#51cf66', width=2)
draw.text((gy91_x + 175, gy91_y + 77), "BMP280", fill='#51cf66', font=font_pin)

# ============= GY-91 Pins (right side) =============
gy91_pins = [
    ("VIN", C_VCC, "5V"),
    ("GND", C_GND, "GND"),
    ("3V3", '#ffd43b', "NC (Output)"),
    ("SCL", C_LINE, "GPIO4"),
    ("SDA", C_LINE, "GPIO3"),
    ("SAO/SDO", '#ff6b6b', "GND → 0x68/0x76"),
    ("NCS", '#888888', "NC (I2C)"),
    ("CSB", '#888888', "NC (I2C)"),
]

gy91_pin_x = gy91_x + gy91_w - 20 - 110
gy91_pin_y_start = gy91_y + 140
gy91_pin_h = 40

for i, (name, color, note) in enumerate(gy91_pins):
    y = gy91_pin_y_start + i * gy91_pin_h
    # Pin rectangle
    draw.rounded_rectangle([gy91_pin_x, y, gy91_pin_x + 110, y + 28], 
                            radius=4, fill=color, outline='#444444', width=1)
    draw.text((gy91_pin_x + 8, y + 3), name, fill=C_PIN_TEXT, font=font_pin)
    
    # Note text (left of pin)
    note_x = gy91_pin_x - 180
    draw.text((note_x, y + 6), note, fill=C_LABEL, font=font_small)

# ============= CONNECTION LINES =============
# SDA: GPIO3 (J3) → GY-91 SDA
sda_gy91_y = gy91_pin_y_start + 4 * gy91_pin_h + 14
# SCL: GPIO4 (J3) → GY91 SCL  
scl_gy91_y = gy91_pin_y_start + 3 * gy91_pin_h + 14

# GND line
gnd_gy91_y = gy91_pin_y_start + 1 * gy91_pin_h + 14
# VIN line
vin_gy91_y = gy91_pin_y_start + 0 * gy91_pin_h + 14

line_start_x = j3_x + 90  # Right edge of J3 pins
line_end_x = gy91_pin_x    # Left edge of GY-91 pins
mid_x = (line_start_x + line_end_x) // 2

# Draw SDA line (blue)
draw.line([(line_start_x, sda_y), (mid_x, sda_y), (mid_x, sda_gy91_y), (line_end_x, sda_gy91_y)], 
          fill=C_LINE, width=3)
draw.ellipse([mid_x - 5, sda_y - 5, mid_x + 5, sda_y + 5], fill=C_LINE)
draw.ellipse([mid_x - 5, sda_gy91_y - 5, mid_x + 5, sda_gy91_y + 5], fill=C_LINE)

# Draw SCL line (cyan)
draw.line([(line_start_x, scl_y), (mid_x + 20, scl_y), (mid_x + 20, scl_gy91_y), (line_end_x, scl_gy91_y)], 
          fill='#00d9ff', width=3)
draw.ellipse([mid_x + 20 - 5, scl_y - 5, mid_x + 20 + 5, scl_y + 5], fill='#00d9ff')
draw.ellipse([mid_x + 20 - 5, scl_gy91_y - 5, mid_x + 20 + 5, scl_gy91_y + 5], fill='#00d9ff')

# Draw GND line (red, from bottom)
gnd_board_y = board_y + board_h - 30
draw.line([(board_x + board_w//2, gnd_board_y), (board_x + board_w//2, gnd_board_y + 40),
           (mid_x - 30, gnd_board_y + 40), (mid_x - 30, gnd_gy91_y), (line_end_x, gnd_gy91_y)], 
          fill=C_GND, width=3)

# Draw VIN line (green, from bottom)
draw.line([(board_x + board_w//2 + 60, gnd_board_y), (board_x + board_w//2 + 60, gnd_board_y + 60),
           (mid_x - 50, gnd_board_y + 60), (mid_x - 50, vin_gy91_y), (line_end_x, vin_gy91_y)], 
          fill=C_VCC, width=3)

# ============= LEGEND =============
legend_x, legend_y = 80, board_y + board_h + 20
draw.rounded_rectangle([legend_x, legend_y, legend_x + 500, legend_y + 90], 
                        radius=8, fill='#0f3460', outline='#533483', width=2)
draw.text((legend_x + 15, legend_y + 10), "接线说明", fill=C_TITLE, font=font_header)

legend_items = [
    (C_LINE, "SDA (GPIO3) → I2C 数据线"),
    ('#00d9ff', "SCL (GPIO4) → I2C 时钟线"),
    (C_GND, "GND → 共地"),
    (C_VCC, "VIN → 5V 供电"),
]

for i, (color, text) in enumerate(legend_items):
    lx = legend_x + 20 + (i % 2) * 240
    ly = legend_y + 40 + (i // 2) * 25
    draw.rectangle([lx, ly, lx + 20, ly + 15], fill=color, outline='#ffffff', width=1)
    draw.text((lx + 28, ly), text, fill=C_LABEL, font=font_small)

# ============= ADDRESS INFO =============
addr_x, addr_y = 620, board_y + board_h + 20
draw.rounded_rectangle([addr_x, addr_y, addr_x + 520, addr_y + 90], 
                        radius=8, fill='#0f3460', outline='#e94560', width=2)
draw.text((addr_x + 15, addr_y + 10), "I2C 地址配置", fill=C_TITLE, font=font_header)
draw.text((addr_x + 20, addr_y + 38), "SAO/SDO = GND → MPU9250: 0x68 | BMP280: 0x76", fill=C_LABEL, font=font_small)
draw.text((addr_x + 20, addr_y + 60), "SAO/SDO = 3.3V → MPU9250: 0x69 | BMP280: 0x77", fill=C_LABEL, font=font_small)

# ============= NOTES =============
note_x, note_y = 80, H - 80
draw.text((note_x, note_y), "注意：GPIO3/4 为自定义 I2C 引脚，避开 ST7789 字库占用的 GPIO9", fill='#888888', font=font_small)
draw.text((note_x, note_y + 18), "3V3 为输出引脚（板载 LDO），可悬空", fill='#888888', font=font_small)

# Save
output_path = 'D:/acela/my_wiki/我的知识库/raw/esp32-hardware/GY91_wiring_diagram.png'
img.save(output_path, 'PNG', dpi=(150, 150))
print(f"Saved: {output_path}")
