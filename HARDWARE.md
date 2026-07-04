# Hardware Configuration (ESP32-S3)

## Wiring Diagram

| Component | Pin | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **OLED (I2C)** | SDA, SCL | GPIO 8, 9 | |
| **Rotary Encoder**| CLK, DT, SW | GPIO 10, 11, 12 | Edge-triggered, non-blocking |
| **Hall Sensor** | Signal | GPIO 4 | **SIDE MOUNT** (height of the bar) |
| **Kill Switch** | Drive | GPIO 5 | P-MOS high-side (requires NPN driver) |
| **Serial Link** | RX, TX | GPIO 6, 7 | Hardware UART1 |
| **4-Pin Fan** | PWM, Tacho | GPIO 14, 13 | 25kHz PWM, 10k Tacho Pull-up |
| **Buzzer** | Signal | GPIO 15 | Active Buzzer |
| **RGB LED** | R, G, B| GPIO 16, 17, 18 | Common Cathode |

## Engineering Principles

### 1. Control & Safety
- **Dual-Core:** PID and hardware interrupts run on Core 1 for zero-jitter timing. UI and Comms run on Core 0.
- **Stall/Decoupling:** If Fan RPM < 100 or Bar RPM < 50 (while Fan > 500), the system enters a safety shutdown and activates the GPIO 5 Kill Switch.
- **Ramping:** Linear RPM ramping prevents inertia-based decoupling.

### 2. S3 Pin Constraints
GPIO 6 and 7 are used for Serial1 to avoid interference with the ESP32-S3 UART0 boot logs. Enable **USB CDC On Boot** in Arduino IDE to use the native USB port for Serial debugging.
