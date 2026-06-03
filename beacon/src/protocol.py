import struct

import config
from adafruit_mcp2515.canio import Message


def parse_packet(packet):
    ident = chr(packet[1])
    b1 = packet[2]
    b2 = packet[3]

    if b1 <= 127:
        value = b1 + (b2 / 100.0)
    else:
        value = ((b1 - 128) * 100) + b2

    return ident, value


def send_to_can(mcp, ident, value):
    try:
        payload = struct.pack("<Bf", ord(ident), float(value))
        mcp.send(Message(id=0x100, data=payload))
    except Exception as e:
        print(f"can error: {e}")


def send_lora(rfm, echook_data, gps=None):
    parts = [f"{k}:{v:.2f}" for k, v in echook_data.items()]

    if gps is not None and gps.has_fix:
        if gps.latitude is not None:
            parts.append(f"lat:{gps.latitude:.5f}")
        if gps.longitude is not None:
            parts.append(f"lon:{gps.longitude:.5f}")
        if gps.speed_knots is not None:
            parts.append(f"spd:{gps.speed_knots:.5f}")
        if gps.satellites is not None:
            parts.append(f"sat:{gps.satellites}")

    if not parts:
        return

    payload_str = ",".join(parts)
    payload_bytes = payload_str.encode("utf-8")
    rfm.send(payload_bytes)

    if config.DEBUG:
        print(f"(( LoRa TX )) -> {payload_str}")
