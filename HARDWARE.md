# DIY Magnetic Stirrer - Final Documentation

## Hardware Pinout (ESP32-S3)

| Component | ESP32-S3 Pin | Note |
| :--- | :--- | :--- |
| **Bio_RX / TX** | GPIO 4 / 5 | Serial1 for master bioreactor comms |
| **Hall Sensor (Bar)** | GPIO 6 | Side-mount (Lateral) |
| **Kill Switch** | GPIO 7 | Controls Gate of P-MOSFET via NPN driver |
| **OLED SDA / SCL** | GPIO 8 / 9 | I2C (0.96" OLED) |
| **Encoder CLK/DT/SW**| GPIO 10,11,12 | Non-blocking rotary input |
| **Fan Tacho** | GPIO 13 | Input (2 PPR typical). **10k Pull-up to 3.3V** |
| **Fan PWM** | GPIO 14 | 25kHz High-frequency control |
| **Buzzer** | GPIO 15 | Active buzzer for alarms |
| **RGB LED (R,G,B)** | GPIO 16,17,18 | Common Anode. **220Ω series resistors req.** |

## User Interface Guide

### 1. Basic Operation
- **Turn Knob:** Adjust target RPM (0-100% power).
- **Short Press Encoder:** Toggle ON/OFF.
- **Display:** Shows real-time RPM (measured by bar-sensor if available, else estimated by fan-tacho).

### 2. Timer Mode
- **Long Press (1s):** Enter "T-SET" mode.
- **Adjust Knob:** Increase/Decrease timer in 5-minute increments.
- **Long Press (1s) Again:** Save timer and exit.
- **Operation:** When started, the stirrer will count down and automatically stop when done.

### 3. Auto-Calibration
- **Extra-Long Press (3s):** Start Calibration.
- **Process:** The stirrer will cycle through 11 power points (50 to 250 PWM) to map motor response.

## Electrical Precautions
- **Common Ground:** GND must be shared between 12V supply, Buck converter, and ESP32.
- **Tacho Signal:** Never connect the fan tacho directly to 12V. Use a 10k pull-up to **3.3V**.
- **LED Protection:** Use 220-330Ω resistors on each color channel of the RGB LED to prevent GPIO damage.

---
*See [CABLAGE.md](CABLAGE.md) for detailed schematics.*
