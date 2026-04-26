
# Air Quality Monitor – ESP32-C3 + SCD41 + ST7735S

Firmware for ESP32-C3 that reads CO₂, temperature, and humidity from an **SCD41** sensor and displays the values on an **ST7735S** colour LCD (160×80).

## Features

- Periodic measurements every 2 seconds
- Large digits (scale 2) for easy reading
- I2C mutex protection (ready for multi‑tasking)
- Clean monospaced 8×16 font (VGA style)

## Hardware

- **MCU**: ESP32-C3 (or any ESP32 with ESP-IDF)
- **Sensor**: Sensirion SCD41 (I2C)
- **Display**: ST7735S (SPI, 160×80)

## Wiring

### SCD41 (I2C)

| SCD41 | ESP32-C3 |
|-------|----------|
| VDD   | 3.3V     |
| GND   | GND      |
| SDA   | GPIO 8   |
| SCL   | GPIO 9   |

*Pull‑ups are enabled in software.*

### ST7735S (SPI)

| ST7735S | ESP32-C3 |
|---------|----------|
| VCC     | 3.3V     |
| GND     | GND      |
| MOSI    | GPIO 4   |
| SCLK    | GPIO 5   |
| CS      | GPIO 10  |
| DC      | GPIO 6   |
| RST     | GPIO 7   |

## Dependencies

- ESP‑IDF v4.4 or newer
- Standard ESP‑IDF components: `driver/i2c`, `driver/spi_master`, `esp_lcd` (includes ST7735 driver)

## Build & Flash

1. Clone the repository:
   ```bash
   git clone https://github.com/Courtesyl/esp32c3-co2-sensor.git
   cd co2-monitor
   ```
2. Set the target (if not already C3):
   ```bash
   idf.py set-target esp32c3
   ```
3. Build:
   ```bash
   idf.py build
   ```
4. Flash and monitor:
   ```bash
   idf.py -p PORT flash monitor
   ```
## How It Works
-I2C bus is initialised at 100 kHz with pull‑ups.

-SCD41 is set to periodic measurement mode (0x21B1).

Every 2 seconds the task _sensor_task_:

--Takes the I2C mutex

--Checks if new data is ready (scd41_data_ready_check)

--Reads CO₂, temperature and humidity with CRC checks

--Updates a framebuffer (RGB565) with text: white labels, cyan values

--Flushes the framebuffer to the ST7735S display

The display orientation is fixed by calling swap_xy, mirror and set_gap – adjust if your panel is mounted differently.

## Code Structure

main.c – startup, framebuffer, font, display driver initialisation, main task.

scd41_drv.c/h – low‑level I2C driver for SCD41: init, CRC check, read measurement, start periodic mode.

Font – 8×16 pixel bitmap characters (ASCII 32–126).

## Notes

First few measurements may be delayed – the sensor needs time to warm up.

Colour format: RGB565 (0xFFFF = white, 0x07FF = cyan, 0x0000 = black).

The framebuffer is 160×80×2 = 25.6 kB – fits comfortably in ESP32‑C3 RAM.

## License

MIT

## Author
Courtesyl

https://github.com/Courtesyl
