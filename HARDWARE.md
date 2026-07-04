# Hardware Configuration (ESP32-S3)

## Wiring Diagram

| Component | Pin | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **OLED (I2C)** | SDA | GPIO 8 | |
| | SCL | GPIO 9 | |
| **Rotary Encoder**| CLK | GPIO 10 | |
| | DT | GPIO 11 | |
| | SW | GPIO 12 | Short: Mode, Long: Start/Stop |
| **Hall Sensor** | Signal | GPIO 4 | Optional: Direct Bar RPM |
| **Kill Switch** | Drive | GPIO 5 | Optional: High-side power cut |
| **Buzzer** | Signal | GPIO 15 | Active Buzzer |
| **RGB LED** | R, G, B| GPIO 16, 17, 18 | Common Cathode |
| **4-Pin Fan** | PWM | GPIO 14 | 25kHz Signal |
| | Tacho | GPIO 13 | **10k Pull-up to 3.3V Required** |
| **Serial Link** | RX | GPIO 6 | Bioreactor Master TX |
| | TX | GPIO 7 | Bioreactor Master RX |

## Important Notes

### 1. UART Pin Mapping
Serial1 is on **GPIO 6 and 7**. This avoids conflicts with the ESP32-S3's native boot logs and flashing pins. Use the Native USB port (GPIO 19/20) for Serial Monitor by enabling **USB CDC On Boot**.

### 2. High-Side Kill Switch (E-STOP)
Connecting GPIO 5 to a P-Channel MOSFET or a dedicated High-side power switch allows the firmware to physically cut the 12V supply to the fan in case of decoupling or stall, providing a true hardware fail-safe.

### 3. Stir Bar Hall Sensor
Placing a Hall Effect sensor (e.g. A3144) near the stirring container allows the firmware to measure the magnetic pulses of the stir bar itself. This enables **True Decoupling Detection** and high-precision PID control.
