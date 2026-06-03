import board

DEBUG = False

# LoRa pins (SPI0)
LORA_SCK = board.GP18
LORA_MOSI = board.GP19
LORA_MISO = board.GP16
LORA_CS = board.GP17
LORA_RESET = board.GP20
LORA_FREQ = 869.525
LORA_POWER = 20

# CAN pins (SPI1)
CAN_SCK = board.GP10
CAN_MOSI = board.GP11
CAN_MISO = board.GP8
CAN_CS = board.GP9
CAN_BAUDRATE = 250000

# UART (eChook)
UART_TX = board.GP12
UART_RX = board.GP13
UART_BAUDRATE = 115200

# GPS (Modul) - UART1
GPS_TX = board.GP4
GPS_RX = board.GP5
GPS_BAUDRATE = 9600

# LoRa send interval
LORA_INTERVAL = 0.3
