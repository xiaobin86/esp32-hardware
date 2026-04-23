from PIL import Image, ImageDraw, ImageFont
import os

# Create output directory
os.makedirs('outputs', exist_ok=True)

# Canvas size
W, H = 1400, 1100
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
C_WARN = '#ffa500'

# Try to load font, fallback to default
try:
    font_title = ImageFont.truetype("arial.ttf", 28)
    font_header = ImageFont.truetype("arial.ttf", 18)
    font_pin = ImageFont.truetype("arial.ttf", 13)
    font_small = ImageFont.truetype("arial.ttf", 11)
    font_tiny = ImageFont.truetype("arial.ttf", 10)
except:
    font_title = ImageFont.load_default()
    font_header = ImageFont.load_default()
    font_pin = ImageFont.load_default()
    font_small = ImageFont.load_default()
    font_tiny = ImageFont.load_default()

# Title
draw.text((W//2 - 320, 20), "GY-91 MPU9250+BMP280 Wiring Diagram", fill=C_TITLE, font=font_title)
draw.text((W//2 - 260, 55), "ESP32-S3-DevKitC-1 v1.1  ←→  GY-91 (I2C Mode)", fill=C_LABEL, font=font_header)
draw.text((W//2 - 200, 80), "Official Pin Layout Verified", fill=C_WARN, font=font_small)

# ============= ESP32-S3 Board =============
board_x, board_y = 60, 130
board_w, board_h = 420, 840

# Board body
draw.rounded_rectangle([board_x, board_y, board_x + board_w, board_y + board_h], 
                        radius=15, fill=C_BOARD, outline=C_BOARD_BORDER, width=3)

# Board label
draw.text((board_x + 70, board_y + 12), "ESP32-S3-DevKitC-1 v1.1", fill=C_TITLE, font=font_header)
draw.text((board_x + 100, board_y + 38), "J1 Left (1-22)  |  J3 Right (1-22)", fill=C_LABEL, font=font_small)

# USB port
draw.rounded_rectangle([board_x + 160, board_y + 65, board_x + 260, board_y + 90], 
                        radius=5, fill='#2d2d2d', outline='#555555', width=2)
draw.text((board_x + 180, board_y + 70), "USB-C", fill='#888888', font=font_small)

# J1 Left header pins (actual layout from official doc)
j1_x = board_x + 15
j1_pins = [
    ("1: 3V3", C_VCC), ("2: 3V3", C_VCC), ("3: RST", '#888888'),
    ("4: GPIO4", C_LINE),   # SCL
    ("5: GPIO5", C_PIN), ("6: GPIO6", C_PIN), ("7: GPIO7", C_PIN),
    ("8: GPIO15", C_PIN), ("9: GPIO16", C_PIN), ("10: GPIO17", C_PIN),
    ("11: GPIO18", C_PIN), ("12: GPIO8", C_PIN),
    ("13: GPIO3", C_LINE),   # SDA
    ("14: GPIO46", C_PIN),
    ("15: GPIO9", C_PIN), ("16: GPIO10", C_PIN),
    ("17: GPIO11", C_PIN), ("18: GPIO12", C_PIN),
    ("19: GPIO13", C_PIN), ("20: GPIO14", C_PIN),
    ("21: 5V", C_VCC), ("22: GND", C_GND),
]

# J3 Right header pins (actual layout from official doc)
j3_x = board_x + board_w - 15 - 110
j3_pins = [
    ("1: GND", C_GND), ("2: TX", C_PIN), ("3: RX", C_PIN),
    ("4: GPIO1", C_PIN),    # MAX98357A DIN
    ("5: GPIO2", C_PIN), ("6: GPIO42", C_PIN), ("7: GPIO41", C_PIN),
    ("8: GPIO40", C_PIN), ("9: GPIO39", C_PIN), ("10: GPIO38", C_PIN),
    ("11: GPIO37", C_PIN), ("12: GPIO36", C_PIN), ("13: GPIO35", C_PIN),
    ("14: GPIO0", C_PIN), ("15: GPIO45", C_PIN), ("16: GPIO48", C_PIN),
    ("17: GPIO47", C_PIN),  # MAX98357A BCLK
    ("18: GPIO21", C_PIN),  # MAX98357A LRC
    ("19: GPIO20", C_PIN), ("20: GPIO19", C_PIN),
    ("21: GND", C_GND), ("22: GND", C_GND),
]

# Draw J1 pins
pin_y_start = board_y + 105
pin_h = 34
for i, (name, color) in enumerate(j1_pins):
    y = pin_y_start + i * pin_h
    draw.rounded_rectangle([j1_x, y, j1_x + 110, y + 26], radius=4, fill=color, outline='#444444', width=1)
    draw.text((j1_x + 6, y + 5), name, fill=C_PIN_TEXT, font=font_pin)

# Draw J3 pins
for i, (name, color) in enumerate(j3_pins):
    y = pin_y_start + i * pin_h
    draw.rounded_rectangle([j3_x, y, j3_x + 110, y + 26], radius=4, fill=color, outline='#444444', width=1)
    draw.text((j3_x + 6, y + 5), name, fill=C_PIN_TEXT, font=font_pin)

# Highlight key pins with circles
# GPIO3 (SDA) at J1-13
sda_idx = 12
sda_y = pin_y_start + sda_idx * pin_h + 13
draw.ellipse([j1_x - 10, sda_y - 10, j1_x + 10, sda_y + 10], outline=C_LINE, width=4)

# GPIO4 (SCL) at J1-4
scl_idx = 3
scl_y = pin_y_start + scl_idx * pin_h + 13
draw.ellipse([j1_x - 10, scl_y - 10, j1_x + 10, scl_y + 10], outline='#00d9ff', width=4)

# ============= GY-91 Module =============
gy91_x, gy91_y = 600, 180
gy91_w, gy91_h = 340, 480

# Module body
draw.rounded_rectangle([gy91_x, gy91_y, gy91_x + gy91_w, gy91_y + gy91_h], 
                        radius=12, fill=C_GY91, outline=C_GY91_BORDER, width=4)

# Module label
draw.text((gy91_x + 50, gy91_y + 12), "GY-91 10DOF Module", fill=C_GY91_BORDER, font=font_header)
draw.text((gy91_x + 60, gy91_y + 38), "MPU9250 + BMP280", fill=C_LABEL, font=font_small)

# Chip labels
draw.rounded_rectangle([gy91_x + 30, gy91_y + 70, gy91_x + 140, gy91_y + 100], 
                        radius=5, fill='#2d2d2d', outline='#e94560', width=2)
draw.text((gy91_x + 45, gy91_y + 78), "MPU9250", fill='#e94560', font=font_pin)

draw.rounded_rectangle([gy91_x + 170, gy91_y + 70, gy91_x + 280, gy91_y + 100], 
                        radius=5, fill='#2d2d2d', outline='#51cf66', width=2)
draw.text((gy91_x + 190, gy91_y + 78), "BMP280", fill='#51cf66', font=font_pin)

# GY-91 Pins (right side)
gy91_pins = [
    ("VIN", C_VCC, "5V (Recommend)"),
    ("GND", C_GND, "GND"),
    ("3V3", '#ffd43b', "NC (Output)"),
    ("SCL", '#00d9ff', "J1-4 (GPIO4)"),
    ("SDA", C_LINE, "J1-13 (GPIO3)"),
    ("SAO/SDO", '#ff6b6b', "GND → 0x68/0x76"),
    ("NCS", '#888888', "NC (I2C mode)"),
    ("CSB", '#888888', "NC (I2C mode)"),
]

gy91_pin_x = gy91_x + gy91_w - 20 - 120
gy91_pin_y_start = gy91_y + 140
gy91_pin_h = 42

for i, (name, color, note) in enumerate(gy91_pins):
    y = gy91_pin_y_start + i * gy91_pin_h
    draw.rounded_rectangle([gy91_pin_x, y, gy91_pin_x + 120, y + 30], 
                            radius=4, fill=color, outline='#444444', width=1)
    draw.text((gy91_pin_x + 8, y + 5), name, fill=C_PIN_TEXT, font=font_pin)
    
    note_x = gy91_pin_x - 210
    draw.text((note_x, y + 7), note, fill=C_LABEL, font=font_small)

# ============= CONNECTION LINES =============
line_start_x = j1_x + 110  # Right edge of J1 pins
line_end_x = gy91_pin_x    # Left edge of GY-91 pins
mid_x = (line_start_x + line_end_x) // 2

# SDA line: GPIO3 (J1-13) → GY-91 SDA
sda_gy91_y = gy91_pin_y_start + 4 * gy91_pin_h + 15
draw.line([(line_start_x, sda_y), (mid_x - 30, sda_y), (mid_x - 30, sda_gy91_y), (line_end_x, sda_gy91_y)], 
          fill=C_LINE, width=4)
draw.ellipse([mid_x - 30 - 6, sda_y - 6, mid_x - 30 + 6, sda_y + 6], fill=C_LINE)
draw.ellipse([mid_x - 30 - 6, sda_gy91_y - 6, mid_x - 30 + 6, sda_gy91_y + 6], fill=C_LINE)

# SCL line: GPIO4 (J1-4) → GY-91 SCL
scl_gy91_y = gy91_pin_y_start + 3 * gy91_pin_h + 15
draw.line([(line_start_x, scl_y), (mid_x + 10, scl_y), (mid_x + 10, scl_gy91_y), (line_end_x, scl_gy91_y)], 
          fill='#00d9ff', width=4)
draw.ellipse([mid_x + 10 - 6, scl_y - 6, mid_x + 10 + 6, scl_y + 6], fill='#00d9ff')
draw.ellipse([mid_x + 10 - 6, scl_gy91_y - 6, mid_x + 10 + 6, scl_gy91_y + 6], fill='#00d9ff')

# GND line (from J1-22 bottom area)
gnd_board_y = pin_y_start + 21 * pin_h + 13
gnd_gy91_y = gy91_pin_y_start + 1 * gy91_pin_h + 15
draw.line([(board_x + board_w//2, gnd_board_y + 15), (board_x + board_w//2, gnd_board_y + 50),
           (mid_x - 60, gnd_board_y + 50), (mid_x - 60, gnd_gy91_y), (line_end_x, gnd_gy91_y)], 
          fill=C_GND, width=3)

# VIN line (from J1-21)
vin_board_y = pin_y_start + 20 * pin_h + 13
vin_gy91_y = gy91_pin_y_start + 0 * gy91_pin_h + 15
draw.line([(board_x + board_w//2 + 60, vin_board_y + 15), (board_x + board_w//2 + 60, vin_board_y + 70),
           (mid_x - 80, vin_board_y + 70), (mid_x - 80, vin_gy91_y), (line_end_x, vin_gy91_y)], 
          fill=C_VCC, width=3)

# ============= LEGEND =============
legend_x, legend_y = 60, board_y + board_h + 30
draw.rounded_rectangle([legend_x, legend_y, legend_x + 520, legend_y + 100], 
                        radius=8, fill='#0f3460', outline='#533483', width=2)
draw.text((legend_x + 15, legend_y + 10), "Connection Legend", fill=C_TITLE, font=font_header)

legend_items = [
    (C_LINE, "SDA (GPIO3 @ J1-13) → I2C Data"),
    ('#00d9ff', "SCL (GPIO4 @ J1-4) → I2C Clock"),
    (C_GND, "GND → Common Ground"),
    (C_VCC, "VIN → 5V Power"),
]

for i, (color, text) in enumerate(legend_items):
    lx = legend_x + 20 + (i % 2) * 250
    ly = legend_y + 40 + (i // 2) * 28
    draw.rectangle([lx, ly, lx + 20, ly + 16], fill=color, outline='#ffffff', width=1)
    draw.text((lx + 28, ly + 1), text, fill=C_LABEL, font=font_small)

# ============= ADDRESS INFO =============
addr_x, addr_y = 620, board_y + board_h + 30
draw.rounded_rectangle([addr_x, addr_y, addr_x + 540, addr_y + 100], 
                        radius=8, fill='#0f3460', outline='#e94560', width=2)
draw.text((addr_x + 15, addr_y + 10), "I2C Address Config", fill=C_TITLE, font=font_header)
draw.text((addr_x + 20, addr_y + 38), "SAO/SDO = GND → MPU9250: 0x68 | BMP280: 0x76", fill=C_LABEL, font=font_small)
draw.text((addr_x + 20, addr_y + 62), "SAO/SDO = 3.3V → MPU9250: 0x69 | BMP280: 0x77", fill=C_LABEL, font=font_small)

# ============= OFFICIAL REFERENCE =============
ref_x, ref_y = 60, H - 90
draw.text((ref_x, ref_y), "Official Reference: ESP32-S3-DevKitC-1 v1.1 Pinout", fill=C_WARN, font=font_small)
draw.text((ref_x, ref_y + 18), "https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html", fill='#888888', font=font_tiny)
draw.text((ref_x, ref_y + 34), "Note: GPIO3 is at J1-13 (not J3), GPIO4 is at J1-4 (not J3). Verified against official docs.", fill='#888888', font=font_tiny)

# Save
output_path = 'D:/acela/my_wiki/我的知识库/raw/esp32-hardware/GY91_wiring_diagram.png'
img.save(output_path, 'PNG', dpi=(150, 150))
print(f"Saved: {output_path}")
