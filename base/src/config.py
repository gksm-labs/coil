import board

DEBUG = True

LED = board.LED
LED_INTERVAL = 0.3

# LoRa pins (SPI0)
LORA_SCK = board.GP18
LORA_MOSI = board.GP19
LORA_MISO = board.GP16
LORA_CS = board.GP17
LORA_RESET = board.GP20
LORA_FREQ = 869.525
LORA_TIMEOUT = 1.0

# Wi-Fi Configuration
WIFI_SSID = "Your_WiFi_SSID"
WIFI_PASSWORD = "Your_WiFi_Password"

# InfluxDB Configuration
INFLUX_URL = "http://192.168.1.100:8086"
INFLUX_ORG = "your_org"
INFLUX_BUCKET = "mm"
INFLUX_TOKEN = "your_influx_token"
INFLUX_MEASUREMENT = "echook_data"

GATEWAY_URL = "https://router.live.mm.gksm.sk/upload"
GATEWAY_API_KEY = ""
