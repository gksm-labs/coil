import busio
import config
import digitalio
import displayio
from adafruit_mcp2515 import MCP2515 as CAN
from adafruit_st7789 import ST7789
from fourwire import FourWire


def init_led():
    led = digitalio.DigitalInOut(config.LED_PIN)
    led.direction = digitalio.Direction.OUTPUT
    return led


def init_display():
    displayio.release_displays()
    tft_bl = digitalio.DigitalInOut(config.TFT_BL)
    tft_bl.direction = digitalio.Direction.OUTPUT
    tft_bl.value = True
    spi_tft = busio.SPI(config.TFT_SCK, config.TFT_MOSI, config.TFT_MISO)
    display_bus = FourWire(
        spi_tft,
        command=config.TFT_DC,
        chip_select=config.TFT_CS,
        reset=config.TFT_RST,
    )
    return ST7789(display_bus, width=320, height=240, rotation=90)


def init_can():
    can_cs = digitalio.DigitalInOut(config.CAN_CS)
    can_cs.direction = digitalio.Direction.OUTPUT
    spi_can = busio.SPI(config.CAN_SCK, config.CAN_MOSI, config.CAN_MISO)
    try:
        can_bus = CAN(spi_can, can_cs, baudrate=config.CAN_BAUDRATE)
        print("can success!")
        return can_bus
    except Exception as e:
        print(e)
        return None
