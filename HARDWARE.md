# Hardware Configuration for DIY Magnetic Stirrer (ESP32-S3)

## Wiring Diagram

| Component | Pin | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **OLED (I2C)** | SDA | GPIO 8 | |
| | SCL | GPIO 9 | |
| **Rotary Encoder**| CLK | GPIO 10 | |
| | DT | GPIO 11 | |
| | SW | GPIO 12 | Short: Mode, Long: ON/OFF |
| **Buzzer** | Signal | GPIO 15 | Active Buzzer |
| **RGB LED** | R, G, B| GPIO 16, 17, 18 | Common Cathode |
| **4-Pin Fan** | PWM | GPIO 14 | 25kHz Signal |
| | Tacho | GPIO 13 | **10k Pull-up to 3.3V Required** |
| **Serial Link** | RX | GPIO 4 | Bioreactor Master TX |
| | TX | GPIO 5 | Bioreactor Master RX |

## Important Interfacing Notes

### 1. UART Communication (Serial1)
The Serial1 link for bioreactor integration has been moved to **GPIO 4 (RX)** and **GPIO 5 (TX)**.
- Avoid using GPIO 43 and 44, as these are the default UART0 pins used for boot logging and firmware flashing.
- If you use the native USB port for debugging, ensure "USB CDC On Boot" is **ENABLED** in your Arduino IDE settings.

### 2. Fan Tachometer Protection
PC Fans have an open-collector tachometer output. To protect the ESP32 and get a valid signal, you **MUST** connect a 10k Ohm resistor between the Tacho pin (GPIO 13) and 3.3V.

### 3. Stall & Decoupling Detection
- **Software Detection:** The current firmware detects if the fan is physically stalled (actual RPM < 100 while target > 25%).
- **Decoupling:** True magnetic decoupling (fan spins but bar stops) is difficult to detect via tachometer alone. For critical applications, refer to the [Software Refinement Guide](software_refinement.md) for jitter analysis techniques.
