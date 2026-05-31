import board

# Display pins
TFT_DC = board.GP8
TFT_CS = board.GP9
TFT_RST = board.GP15
TFT_BL = board.GP13
TFT_SCK = board.GP10
TFT_MOSI = board.GP11
TFT_MISO = board.GP12

# CAN bus pins
CAN_CS = board.GP1
CAN_SCK = board.GP2
CAN_MOSI = board.GP3
CAN_MISO = board.GP0
CAN_BAUDRATE = 250000

# Status LED
LED_PIN = board.LED

# Application constants
MAX_SPEED = 100.0
MAX_CURRENT = 150.0
MAX_TEMP = 80.0
