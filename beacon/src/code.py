import time

import config
import hardware
import protocol

rfm = hardware.init_lora()
mcp = hardware.init_can()
uart = hardware.init_uart()
gps = hardware.init_gps()
led = hardware.init_led()

uart_buffer = bytearray()
echook_data = {}
last_lora_tx = time.monotonic()

led_last = time.monotonic()
led_state = False

while True:
    if gps is not None:
        gps.update()

    if uart.in_waiting > 0:
        uart_buffer.extend(uart.read(uart.in_waiting))

        while b"{" in uart_buffer:
            start_idx = uart_buffer.index(b"{")
            uart_buffer = uart_buffer[start_idx:]

            if len(uart_buffer) < 5:
                break

            if uart_buffer[4] == 125:
                packet = uart_buffer[:5]
                ident, value = protocol.parse_packet(packet)
                echook_data[ident] = value
                if config.DEBUG:
                    print(f"echook: id='{ident}', value={value:.2f}")

                if mcp is not None:
                    protocol.send_to_can(mcp, ident, value)

                uart_buffer = uart_buffer[5:]
            else:
                uart_buffer = uart_buffer[1:]

    if time.monotonic() - last_lora_tx >= config.LORA_INTERVAL:
        if rfm is not None:
            protocol.send_lora(rfm, echook_data, gps)

        last_lora_tx = time.monotonic()

    if time.monotonic() - led_last >= config.LED_INTERVAL:
        led_state = not led_state
        led.value = led_state
        led_last = time.monotonic()

    time.sleep(0.01)
