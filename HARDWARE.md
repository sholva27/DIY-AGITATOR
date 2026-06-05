# Hardware Configuration for DIY Magnetic Stirrer

This document outlines the hardware setup for the ESP32-S3 based Magnetic Stirrer.

## Components
- **Microcontroller:** ESP32-S3
- **Fan:** 4-Pin PC Cooling Fan (12V)
- **Display:** 0.96" OLED I2C (SSD1306)
- **Input:** Rotary Encoder (with push button)
- **Power:** 12V DC Power Supply
- **Voltage Regulation:** 12V to 5V/3.3V Step-down (Buck) converter for the ESP32

## Wiring Diagram

| Component | Pin | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **OLED** | VCC | 3.3V | |
| | GND | GND | |
| | SDA | GPIO 8 | Default I2C SDA |
| | SCL | GPIO 9 | Default I2C SCL |
| **Rotary Encoder** | CLK | GPIO 10 | |
| | DT | GPIO 11 | |
| | SW | GPIO 12 | Push button |
| | VCC | 3.3V | |
| | GND | GND | |
| **4-Pin Fan** | Pin 1 (GND) | GND | Common ground with ESP32 |
| | Pin 2 (12V) | +12V | Direct from power supply |
| | Pin 3 (Tacho)| GPIO 13 | Use 10k Pull-up to 3.3V |
| | Pin 4 (PWM)  | GPIO 14 | 25kHz PWM Signal |
| **Buzzer** | Positive | GPIO 15 | Active Buzzer (5V or 3.3V) |
| | Negative | GND | |

## Interfacing Recommendations

### 1. Fan PWM Control
PC Fans expect a PWM frequency of approximately **25kHz**. The ESP32-S3's LEDC peripheral is perfect for this. While the fan is 12V, the PWM input is usually compatible with 3.3V logic levels.

### 2. Fan Tachometer (RPM)
The Tachometer output is typically an open-collector signal. This means it "floats" when high and connects to GND when low.
- **Recommendation:** Connect a 10k ohm resistor between the ESP32 3.3V pin and the Tacho GPIO (GPIO 13). This ensures the signal stays at 3.3V when the fan isn't pulling it down, protecting the ESP32.

### 3. Power Supply
- Use a **12V DC adapter** (at least 1A).
- Power the fan directly from the 12V rail.
- Use a **Buck Converter** to drop 12V to 5V to power the ESP32-S3 via its VIN/5V pin.

### 4. Bioreactor Integration (Wired)
- To connect to the Bioreactor ESP32 via wires, use **Serial (UART)**.
- Connect Bioreactor TX -> Stirrer RX (e.g., GPIO 44)
- Connect Bioreactor RX -> Stirrer TX (e.g., GPIO 43)
- **Important:** Ensure both ESP32s share a common Ground.
