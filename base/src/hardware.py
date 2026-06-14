import ssl

import adafruit_ntp
import adafruit_requests
import adafruit_rfm9x
import busio
import config
import digitalio
import rtc
import socketpool
import wifi


def init_lora():
    spi = busio.SPI(config.LORA_SCK, MOSI=config.LORA_MOSI, MISO=config.LORA_MISO)
    cs = digitalio.DigitalInOut(config.LORA_CS)
    rst = digitalio.DigitalInOut(config.LORA_RESET)
    try:
        rfm = adafruit_rfm9x.RFM9x(spi, cs, rst, config.LORA_FREQ)
        print("lora success!")
        return rfm
    except Exception as e:
        print(f"lora error: {e}")
        return None


def init_wifi_requests():
    try:
        print(f"Connecting to WiFi: {config.WIFI_SSID}...")
        wifi.radio.connect(config.WIFI_SSID, config.WIFI_PASSWORD)
        print(f"WiFi connected! IP: {wifi.radio.ipv4_address}")

        pool = socketpool.SocketPool(wifi.radio)

        ntp = adafruit_ntp.NTP(pool, tz_offset=0)
        rtc.RTC().datetime = ntp.datetime

        ssl_context = ssl.create_default_context()
        ssl_context.load_verify_locations(cadata="")
        requests = adafruit_requests.Session(pool, ssl_context)

        print("requests success!")
        return requests
    except Exception as e:
        print(f"WiFi/Requests error: {e}")
        return None


def init_led():
    led = digitalio.DigitalInOut(config.LED)
    led.direction = digitalio.Direction.OUTPUT
    return led
