import digitalio
import board
import busio
import adafruit_rfm9x
import time

spi = busio.SPI(board.GP18, MOSI=board.GP19, MISO=board.GP16)
cs = digitalio.DigitalInOut(board.GP17)
reset = digitalio.DigitalInOut(board.GP20)

rfm = adafruit_rfm9x.RFM9x(spi, cs, reset, 869.525)
rfm.tx_power = 20

print("TX start")

while True:
    rfm.send(bytes("hello", "utf-8"))
    print("jo")
    time.sleep(1)

