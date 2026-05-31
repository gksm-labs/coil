import adafruit_rfm9x
import busio
import config
import digitalio


def init_lora():
    spi = busio.SPI(config.LORA_SCK, MOSI=config.LORA_MOSI, MISO=config.LORA_MISO)
    cs = digitalio.DigitalInOut(config.LORA_CS)
    rst = digitalio.DigitalInOut(config.LORA_RESET)
    try:
        rfm = adafruit_rfm9x.RFM9x(spi, cs, rst, config.LORA_FREQ)
        print("lora success!")
        return rfm
    except Exception as e:
        print(e)
        return None
