import time

import config
import hardware

rfm = hardware.init_lora()

while True:
    if rfm is None:
        time.sleep(1)
        continue

    packet = rfm.receive(timeout=config.LORA_TIMEOUT)

    if packet is None:
        continue

    try:
        text = str(packet, "ascii")
    except Exception:
        text = packet.hex()

    rssi = rfm.last_rssi
    print(f"DATA: {text} | RSSI: {rssi}")
