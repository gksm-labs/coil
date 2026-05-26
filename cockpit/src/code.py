from machine import Pin, SPI, PWM
import framebuf
import time
import os
import random

from display import LCD_2inch8

def draw_large_text(lcd, text, x, y, scale, color):
    """
    Vykreslí zväčšený text pomocou framebuffera.
    """
    text = str(text)
    w = len(text) * 8
    h = 8
    sm_buf = bytearray(w * h * 2)
    sm_fb = framebuf.FrameBuffer(sm_buf, w, h, framebuf.RGB565)
    sm_fb.fill(0x0000) # Čierne pozadie
    sm_fb.text(text, 0, 0, color)
    
    for sy in range(h):
        for sx in range(w):
            c = sm_fb.pixel(sx, sy)
            if c != 0x0000: # Ak je to pixel textu
                lcd.fill_rect(x + sx * scale, y + sy * scale, scale, scale, c)

def draw_gauge(lcd, x, y, width, height, value, max_value, color, bg_color):
    """
    Vykreslí horizontálny progress bar (ukazovateľ).
    """
    # Rámik
    lcd.rect(x, y, width, height, lcd.GRAY)
    # Vnútro pozadie
    lcd.fill_rect(x+1, y+1, width-2, height-2, bg_color)
    
    # Výpočet šírky vyplnenej časti
    if value > max_value: value = max_value
    if value < 0: value = 0
    fill_width = int((value / max_value) * (width - 4))
    
    # Vyplnenie hodnoty
    if fill_width > 0:
        lcd.fill_rect(x+2, y+2, fill_width, height-4, color)


# --- HLAVNÁ SLUČKA (MAIN) ---

if __name__=='__main__':
    LCD = LCD_2inch8()
    LCD.bl_ctrl(100) # Jas na maximum pre viditeľnosť na slnku
    
    # Počiatočné hodnoty (Zatiaľ pre simuláciu)
    voltage = 390.5
    current = 0.0
    temp_bat = 35.2

    # Konfigurácia maximálnych hodnôt pre grafy
    MAX_VOLTAGE = 600.0 
    MAX_CURRENT = 150.0
    MAX_TEMP = 80.0

    while True:
        # 1. Čítanie senzorov (Simulácia - neskôr prepoj na CAN zbernicu)
        voltage += random.uniform(-0.5, 0.5)
        current = abs(current + random.uniform(-5.0, 5.0))
        temp_bat += random.uniform(-0.1, 0.5)
        
        if current > MAX_CURRENT: current = MAX_CURRENT
        if voltage > MAX_VOLTAGE: voltage = MAX_VOLTAGE
        
        LCD.fill(LCD.BLACK)
        
        LCD.text("VOLTAGE (V)", 10, 10, LCD.GRAY)
        vol_color = LCD.YELLOW if voltage > 300 else LCD.RED
        
        draw_large_text(LCD, f"{voltage:>5.1f}", 10, 25, 4, vol_color)
        draw_gauge(LCD, 175, 25, 135, 32, voltage, MAX_VOLTAGE, vol_color, LCD.DARK_GRAY)

        LCD.text("CURRENT (A)", 10, 90, LCD.GRAY)
        draw_large_text(LCD, f"{current:>5.1f}", 10, 105, 4, LCD.CYAN)
        draw_gauge(LCD, 175, 105, 135, 32, current, MAX_CURRENT, LCD.CYAN, LCD.DARK_GRAY)

        LCD.text("BAT TEMP (*C)", 10, 170, LCD.GRAY)
        if temp_bat < 45.0:
            temp_color = LCD.GREEN
        elif temp_bat < 60.0:
            temp_color = LCD.ORANGE
        else:
            temp_color = LCD.RED
            
        draw_large_text(LCD, f"{temp_bat:>5.1f}", 10, 185, 4, temp_color)
        draw_gauge(LCD, 175, 185, 135, 32, temp_bat, MAX_TEMP, temp_color, LCD.DARK_GRAY)

        LCD.hline(0, 80, 320, LCD.DARK_GRAY)
        LCD.hline(0, 160, 320, LCD.DARK_GRAY)

        # 3. Odoslanie hotového obrazu z RAM na reálny displej
        LCD.show_up()

