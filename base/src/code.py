import board
import busio
import digitalio
import adafruit_rfm9x

spi = busio.SPI(board.GP18, MOSI=board.GP19, MISO=board.GP16)
cs = digitalio.DigitalInOut(board.GP17)
reset = digitalio.DigitalInOut(board.GP20)

rfm = adafruit_rfm9x.RFM9x(spi, cs, reset, 869.525)

print("RX start")

while True:
    packet = rfm.receive(timeout=5.0)
    if packet is None:
        print("nothing...")
    else:
        try:
            text = str(packet, "ascii")
            print(f"received: {text}")
        except:
            print(f"received (raw): {packet}")
        print(f"RSSI: {rfm.last_rssi} dB")

