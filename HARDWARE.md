# DIY Magnetic Stirrer - Final Documentation

## Hardware Pinout (ESP32-S3)

| Component | ESP32-S3 Pin | Note |
| :--- | :--- | :--- |
| **Hall Sensor (Bar)** | GPIO 4 | Side-mount (Lateral) to avoid drive magnets |
| **Kill Switch** | GPIO 5 | Controls Gate of P-MOSFET via NPN driver |
| **Bio_RX / TX** | GPIO 6 / 7 | Serial1 for master bioreactor comms |
| **OLED SDA / SCL** | GPIO 8 / 9 | I2C (0.96" OLED) |
| **Encoder CLK/DT/SW**| GPIO 10,11,12 | Non-blocking rotary input |
| **Fan Tacho** | GPIO 13 | Input for RPM calculation (2 PPR typical) |
| **Fan PWM** | GPIO 14 | 25kHz High-frequency control |
| **Buzzer** | GPIO 15 | Active buzzer for alarms |
| **RGB LED (R,G,B)** | GPIO 16,17,18 | Common Anode (255-val in code) |

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
- **Note:** Do not touch the stirrer during this 30-second process.

## Wiring Tips

### 1. The Lateral Hall Sensor
**Crucial:** Do not place the Hall sensor directly under the fan. It will pick up the magnetic poles of the fan's motor.
- Mount the sensor on the **side** of the container.
- Use a 10k pull-up resistor if the sensor is Open-Collector.

### 2. High-Side Power Cut (Safety)
To prevent "creeping" or MOSFET leakage when the system is OFF:
- Use a P-Channel MOSFET on the 12V rail.
- Use a small NPN transistor (like 2N2222) to pull the P-MOSFET gate to GND when GPIO 5 is HIGH.

## Visual & Audible Indicators
- **Green (Solid):** Active & Stirring.
- **Yellow (Flashing):** Timer Finished (Buzzer will beep).
- **Blue (Solid):** Remote Lock (Bioreactor in control).
- **Red (Flashing):** Error (Stall or Decoupled).
- **Dim White:** Standby / IDLE.
