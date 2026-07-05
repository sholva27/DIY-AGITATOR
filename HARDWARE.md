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

## 4-Wire Fan Control Logic

Standard 4-wire PC fans are used for precise speed control and feedback.
- **PWM Control:** The speed is regulated via a **25kHz** high-frequency PWM signal on GPIO 14. This frequency is within the standard Intel specification for PC fans to avoid audible "whining" noise.
- **Speed Feedback:** The tachometer signal (GPIO 13) provides real-time RPM data (typically 2 pulses per revolution).
- **Kill Switch (GPIO 7):** This is a critical safety and control feature.
    - **Total Stop:** Many 4-wire fans have an internal "minimum floor" and will continue to spin slowly even at 0% PWM. The Kill Switch cuts the 12V rail via a MOSFET to ensure a 0 RPM absolute stop.
    - **Safety Fail-safe:** Some fans default to 100% speed if the PWM signal is lost or disconnected. The Kill Switch provides a hardware-level override to shut down the motor regardless of the PWM state.

## Power Supply Specifications

The system requires a **Regulated 12V DC Switching Power Supply**.

- **Voltage:** 12V DC (Regulated mandatory to avoid motor noise issues).
- **Current Rating:**
    - **12V / 1A (Minimum):** Only for standard low-power 120mm fans (<0.2A).
    - **12V / 2A (Recommended):** Covers most high-pressure lab fans and logic overhead.
    - **12V / 3A (Heavy Duty):** For server-grade fans (>0.5A) with high static pressure.
- **Connector:** Standard 5.5 x 2.1mm DC Barrel Jack.
- **Note on Surges:** PC fans can draw 2-3x their nominal current during the first few milliseconds of startup. An undersized power supply will cause a "brown-out" (voltage drop), resetting the ESP32.

## User Interface Guide

### 1. Basic Operation
- **Turn Knob:** Adjust target RPM (0-100% power).
- **Short Press Encoder:** Toggle ON/OFF.
- **Display:** Shows real-time RPM (measured by bar-sensor if available, else estimated by fan-tacho).

### 2. Timer Mode
- **Long Press (1s):** Enter "T-SET" mode (or unlock remote mode).
- **Adjust Knob:** Increase/Decrease timer in 5-minute increments.
- **Long Press (1s) Again:** Save timer and exit.

### 3. Auto-Calibration
- **Extra-Long Press (3s):** Start Calibration to map motor response.

## Electrical Precautions
- **Common Ground:** GND must be shared between 12V supply, Buck converter, and ESP32.
- **Tacho Signal:** Never connect the fan tacho directly to 12V. Use a 10k pull-up to **3.3V**.
- **LED Protection:** Use 220-330Ω resistors on each color channel of the RGB LED.

---
*See [CABLAGE.md](CABLAGE.md) for detailed schematics.*
