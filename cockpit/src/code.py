import random
import struct
import time

import config
import hardware
import ui
from adafruit_mcp2515.canio import Message

led = hardware.init_led()
display = hardware.init_display()
can_bus = hardware.init_can()

(
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
) = ui.build_dashboard(display)

speed = 0.0
voltage = 0.0
current = 0.0
temp1 = 0.0
temp2 = 0.0

can_listener = None
if can_bus is not None:
    can_listener = can_bus.listen(timeout=0)

last_display_update = time.monotonic()
print("active")

while True:
    can_packet_received = False

    if can_listener is not None:
        try:
            for _ in range(can_listener.in_waiting()):
                msg = can_listener.receive()
                if isinstance(msg, Message) and msg.id == 0x100 and len(msg.data) == 5:
                    ident_int, value = struct.unpack("<Bf", msg.data)
                    ident = chr(ident_int)
                    if ident == "s":
                        speed = value
                    elif ident == "v":
                        voltage = value
                    elif ident == "i":
                        current = value
                    elif ident == "a":
                        temp1 = value
                    elif ident == "b":
                        temp2 = value
                    can_packet_received = True
        except Exception as e:
            print("can error:", e)

    if can_bus is None:
        speed = abs(speed + random.uniform(-4.0, 4.0))
        if speed > config.MAX_SPEED:
            speed = config.MAX_SPEED

        if voltage <= 0:
            voltage = 395.0
        voltage -= random.uniform(0.001, 0.01)

        current = (speed * 1.2) + random.uniform(-5.0, 5.0)
        if current < 0:
            current = 0

        if temp1 <= 0:
            temp1 = 32.0
        temp1 += random.uniform(-0.02, 0.05)
        if temp1 > config.MAX_TEMP:
            temp1 = config.MAX_TEMP

    now = time.monotonic()
    if now - last_display_update >= 0.15:
        last_display_update = now

        is_demo = can_bus is None
        bat_pct = ui.calculate_battery_pct(voltage)
        max_temp = max(temp1, temp2)

        lbl_demo.text = "DEMO" if is_demo else ""
        val_speed.text = f"{int(speed):>3}"
        val_battery.text = f"{bat_pct:>3}%"
        val_voltage.text = f"{voltage:>5.1f}V"
        val_current.text = f"{current:>5.1f}A"
        val_temp.text = f"{max_temp:>5.1f}C"

        val_battery.color = 0xFF0000 if bat_pct < 20 else 0x00FF00

        ui.update_bar(bar_battery, bat_pct, 100)
        ui.update_bar(bar_voltage, max(0.0, voltage - 320.0), 90.0)
        ui.update_bar(bar_current, current, config.MAX_CURRENT)
        ui.update_bar(bar_temp, max_temp, config.MAX_TEMP)

        led.value = not led.value
