import time

import config
import hardware
import protocol

rfm = hardware.init_lora()
requests = hardware.init_wifi_requests()
led = hardware.init_led()

led_last = time.monotonic()
led_state = False

while True:
    if time.monotonic() - led_last >= config.LED_INTERVAL:
        led_state = not led_state
        led.value = led_state
        led_last = time.monotonic()

    if rfm is None:
        time.sleep(1)
        rfm = hardware.init_lora()
        continue

    if requests is None:
        time.sleep(5)
        requests = hardware.init_wifi_requests()
        continue

    packet = rfm.receive(timeout=config.LORA_TIMEOUT)

    if packet is None:
        continue

    try:
        text = str(packet, "utf-8")
    except Exception:
        text = packet.hex()
        if config.DEBUG:
            print(f"Decode error. Hex: {text}")
        continue

    rssi = rfm.last_rssi

    if config.DEBUG:
        print(f"DATA: {text} | RSSI: {rssi}")

    protocol.process_and_send(requests, text, rssi)

    time.sleep(0.01)
