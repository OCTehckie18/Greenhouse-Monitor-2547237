# Greenhouse Monitor 2547237

An ESP32-based smart greenhouse controller built with PlatformIO and Arduino. The project reads temperature, humidity, and soil moisture, displays local status on a 16x2 I2C LCD, and controls a servo-powered ventilation flap from either a 4x4 keypad or Blynk IoT.

## Features

- DHT22 temperature and humidity monitoring
- Analog soil-moisture input (a potentiometer simulates the sensor in Wokwi)
- Blynk telemetry and remote vent-angle control
- Servo vent control from 0 to 180 degrees
- 16x2 I2C LCD status display
- Keypad calculator mode and local vent-control mode
- Wokwi simulation configuration in `diagram.json`

## Hardware and pinout

| Component | ESP32 pin |
| --- | --- |
| DHT22 data | GPIO 4 |
| Soil-moisture analog signal | GPIO 34 |
| Servo PWM | GPIO 18 |
| LCD SDA | GPIO 21 |
| LCD SCL | GPIO 22 |
| Keypad rows R1-R4 | GPIO 13, 14, 27, 26 |
| Keypad columns C1–C4 | GPIO 25, 33, 32, 19 |

The LCD uses I2C address `0x27`. Power the LCD and servo appropriately and connect all grounds together.

## Blynk datastreams

Configure these virtual pins in the Blynk template:

| Pin | Value |
| --- | --- |
| V0 | Temperature in degrees C |
| V1 | Relative humidity in % |
| V2 | Soil moisture in % |
| V3 | Vent angle in degrees; slider input and current-position feedback |

Before deploying, replace the Blynk template credentials and Wi-Fi settings in `src/main.cpp`. Do not commit real authentication tokens or passwords to a public repository; rotate the token if it has already been exposed.

## Keypad controls

In calculator mode:

- `A` = addition, `B` = subtraction, `C` = multiplication, `D` = division
- `#` = calculate when an expression is complete
- `*` = clear
- Press `#` on an empty or completed screen to enter vent mode

In vent mode:

- `1` = 0 degrees
- `2` = 90 degrees
- `3` = 180 degrees
- `#` = return to calculator mode

## Build and upload

Install [PlatformIO](https://platformio.org/) and run from the project directory:

```bash
pio run
pio run --target upload
pio device monitor --baud 115200
```

The `esp32dev` environment targets an ESP32 DevKit board and installs the required libraries automatically:

- Blynk
- DHT sensor library
- Adafruit Unified Sensor
- Keypad
- LiquidCrystal_I2C
- ESP32Servo

## Run in Wokwi

The repository includes the circuit definition and Wokwi firmware paths. Build the firmware first with `pio run`, then open the project in Wokwi and start the simulation. The potentiometer represents the soil-moisture sensor, and the Wokwi access point is configured as `Wokwi-GUEST`.

## Project structure

```text
src/main.cpp       Application firmware
platformio.ini     Board, framework, monitor, and library configuration
diagram.json       Wokwi circuit definition
wokwi.toml         Wokwi firmware/ELF paths
```

## License

No license has been specified yet.
