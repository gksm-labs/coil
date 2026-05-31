import adafruit_rfm9x
import busio
import config
import digitalio
from adafruit_mcp2515 import MCP2515


def init_lora():
    spi = busio.SPI(config.LORA_SCK, MOSI=config.LORA_MOSI, MISO=config.LORA_MISO)
    cs = digitalio.DigitalInOut(config.LORA_CS)
    rst = digitalio.DigitalInOut(config.LORA_RESET)
    try:
        rfm = adafruit_rfm9x.RFM9x(spi, cs, rst, config.LORA_FREQ)
        rfm.tx_power = config.LORA_POWER
        print("lora success!")
        return rfm
    except Exception as e:
        print(e)
        return None


def init_can():
    cs = digitalio.DigitalInOut(config.CAN_CS)
    cs.switch_to_output()
    spi = busio.SPI(config.CAN_SCK, MOSI=config.CAN_MOSI, MISO=config.CAN_MISO)
    try:
        mcp = MCP2515(spi, cs, baudrate=config.CAN_BAUDRATE)
        print("can success!")
        return mcp
    except Exception as e:
        print(e)
        return None


def init_uart():
    uart = busio.UART(config.UART_TX, config.UART_RX, baudrate=config.UART_BAUDRATE)
    print("uart success!")
    return uart
