# DIY Magnetic Stirrer for Lab & Bioreactor

This project implements a high-precision, feature-rich Magnetic Stirrer using an **ESP32-S3**. It is designed to work as a standalone laboratory tool or as an integrated stirring node for a larger Bioreactor system.

## 🚀 Key Features

- **Precision Speed Control:** 4-pin PC Fan control via 25kHz PWM.
- **RPM Feedback:** Real-time RPM measurement using the fan's tachometer.
- **Advanced Safety:**
    - **Soft Start/Ramping:** Prevents magnetic decoupling during acceleration.
    - **Decoupling Detection:** Automatically stops and alarms if the magnetic bar slips or the fan stalls.
- **Integrated Timer:** Set a stir duration with an audible alarm (Buzzer) when finished.
- **OLED UI:** 0.96" I2C display showing Target %, Actual RPM, Timer, and System Status.
- **Bioreactor Ready:**
    - **ESP-NOW:** Wireless control for cable-free integration.
    - **UART Serial:** Wired control for high-reliability lab environments.

## 📁 Project Documentation

| File | Description |
| :--- | :--- |
| [HARDWARE.md](HARDWARE.md) | Wiring diagrams, component list, and interfacing tips. |
| [Bioreactor integration.md](Bioreactor%20integration.md) | How to link this project with `Bioreacteur_PI` and `FLUOreacteur`. |
| [research.md](research.md) | Technical specs on PWM, magnetic coupling, and design inspirations. |
| [roadmap.md](roadmap.md) | Future development phases (PID control, Web Dashboard, etc.). |
| [debugging.md](debugging.md) | Troubleshooting guide for common issues. |
| [proposals.md](proposals.md) | Advanced improvement suggestions (in French). |
| [sensor_comparison.md](sensor_comparison.md) | Analysis of ACS712 vs. INA219 for current monitoring. |

## 🛠️ Getting Started

### 1. Hardware Setup
Follow the wiring guide in [HARDWARE.md](HARDWARE.md). Ensure your 4-pin fan is powered by a dedicated 12V source and shares a common ground with the ESP32.

### 2. Required Libraries
- `Adafruit_SSD1306`
- `Adafruit_GFX`
- `Wire`
- `WiFi` & `esp_now` (Standard ESP32 libraries)

### 3. Flashing
1. Open `stirrer_project/stirrer_project.ino` in the Arduino IDE.
2. Select your **ESP32-S3** board.
3. If you are using ESP32 Arduino Core 3.0+, the code will automatically adapt.
4. Upload to your board.
5. Note the **MAC Address** displayed on the OLED at startup for ESP-NOW pairing.

## 🤝 Bioreactor Ecosystem
This project is part of a larger ecosystem of DIY lab tools:
- **Bioreacteur_ESP32_PI:** Control system for pH, Temp, and O2.
- **NADAH_FLUOreacteur_ESP32:** Fluorescence measurement system.

See [Bioreactor integration.md](Bioreactor%20integration.md) for more details on cross-project communication.
