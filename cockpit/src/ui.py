import displayio
import terminalio
from adafruit_display_text import label


def create_text(group, text, x, y, scale=2, color=0xFFFFFF):
    text_group = displayio.Group(scale=scale, x=x, y=y)
    text_area = label.Label(terminalio.FONT, text=text, color=color)
    text_group.append(text_area)
    group.append(text_group)
    return text_area


def create_bar(group, width, height, x, y, bg_color=0x222222, fg_color=0x00FF00):
    bmp = displayio.Bitmap(width, height, 2)
    pal = displayio.Palette(2)
    pal[0] = bg_color
    pal[1] = fg_color
    group.append(displayio.TileGrid(bmp, pixel_shader=pal, x=x, y=y))
    return bmp


def update_bar(bmp, value, max_value):
    bmp.fill(0)
    if max_value <= 0:
        return
    fill = max(0, min(int((value / max_value) * bmp.width), bmp.width))
    for x in range(fill):
        for y in range(bmp.height):
            bmp[x, y] = 1


def calculate_battery_pct(volt):
    if volt <= 0:
        return 0
    if volt < 15.0:
        v_min, v_max = 10.5, 14.4
    elif volt < 30.0:
        v_min, v_max = 21.0, 29.4
    elif volt < 60.0:
        v_min, v_max = 42.0, 58.8
    else:
        v_min, v_max = 320.0, 410.0
    return max(0, min(100, int(((volt - v_min) / (v_max - v_min)) * 100)))


def build_dashboard(display):
    splash = displayio.Group()
    try:
        display.root_group = splash
    except AttributeError:
        display.show(splash)

    create_text(splash, "SPEED", x=20, y=15, scale=1, color=0x888888)
    create_text(splash, "km/h", x=135, y=65, scale=2, color=0x888888)
    create_text(splash, "BATTERY", x=200, y=15, scale=1, color=0x888888)

    div_bmp = displayio.Bitmap(320, 2, 1)
    div_pal = displayio.Palette(1)
    div_pal[0] = 0x444444
    splash.append(displayio.TileGrid(div_bmp, pixel_shader=div_pal, x=0, y=105))

    create_text(splash, "VOLTAGE", x=10, y=120, scale=1, color=0x888888)
    create_text(splash, "CURRENT", x=115, y=120, scale=1, color=0x888888)
    create_text(splash, "BAT TEMP", x=220, y=120, scale=1, color=0x888888)

    val_speed = create_text(splash, "  0", x=15, y=35, scale=6, color=0xFFFFFF)
    val_battery = create_text(splash, "---%", x=200, y=35, scale=4, color=0x00FF00)
    lbl_demo = create_text(splash, "", x=120, y=15, scale=2, color=0xFF0000)
    val_voltage = create_text(splash, "---.-V", x=10, y=140, scale=2, color=0xFFFF00)
    val_current = create_text(splash, "---.-A", x=115, y=140, scale=2, color=0x00FFFF)
    val_temp = create_text(splash, "--.-C", x=220, y=140, scale=2, color=0xFF3333)
    bar_battery = create_bar(
        splash, width=100, height=15, x=200, y=75, fg_color=0x00FF00
    )
    bar_voltage = create_bar(splash, width=85, height=8, x=10, y=165, fg_color=0xFFFF00)
    bar_current = create_bar(
        splash, width=85, height=8, x=115, y=165, fg_color=0x00FFFF
    )
    bar_temp = create_bar(splash, width=85, height=8, x=220, y=165, fg_color=0xFF3333)

    return (
        val_speed,
        val_battery,
        lbl_demo,
        val_voltage,
        val_current,
        val_temp,
        bar_battery,
        bar_voltage,
        bar_current,
        bar_temp,
    )
