import board
import math
import busio
import displayio
import random
import time
from adafruit_display_shapes.line import Line
from fourwire import FourWire

from adafruit_st7789 import ST7789

displayio.release_displays()

spi = busio.SPI(clock = board.GP14, MOSI = board.GP15)
while not spi.try_lock():
    pass
spi.configure(baudrate=24_000_000, phase=1, polarity=1)
spi.unlock()

display_bus = FourWire(
    spi,
    command=board.GP10,
    chip_select=board.GP2,
    reset=board.GP11,
    baudrate = 24_000_000,
    polarity = 1,
    phase = 1)

display = ST7789(display_bus, width=240, height=240, rowstart=80, bgr=True)

if 0:
    splash = displayio.Group()
    display.root_group = splash
    color_bitmap = displayio.Bitmap(240, 240, 1)
    color_palette = displayio.Palette(1)
    color_palette[0] = 0xFF0000

    bg_sprite = displayio.TileGrid(color_bitmap,
                                   pixel_shader=color_palette,
                                   x=0, y=0)
    splash.append(bg_sprite)

if 1:
    group = displayio.Group()
    display.root_group = group

    w, h = display.width, display.height

    xys = []
    for i in range(30):
        th = i * 2 * math.pi / 30
        r = w / 2
        x, y = (w / 2 + r * math.sin(th), h / 2 + r * math.cos(th))
        xys.append((int(x), int(y)))

    for i in range(30):
        (x1, y1) = xys[i]
        (x2, y2) = xys[(i + 7) % 30]
        color = random.randint(0, 0xFFFFFF)
        group.append(Line(x1, y1, x2, y2, color))
        display.refresh(minimum_frames_per_second=0)

if 0:
    group = displayio.Group()
    display.root_group = group

    # Option A: load a .bmp from CIRCUITPY drive
    bitmap = displayio.OnDiskBitmap("/logo.bmp")
    tile_grid = displayio.TileGrid(
        bitmap,
        pixel_shader=bitmap.pixel_shader,
        x=0, y=0
    )
    group.append(tile_grid)

print("ok")
while True:
    pass
