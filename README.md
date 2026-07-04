# DIY Magnetic Stirrer for Lab & Bioreactor

High-performance **ESP32-S3** based magnetic stirrer node for autonomous laboratory applications and bioreactor integration.

## 🚀 Key Features

- **Dual-Loop Precision:**
    - Closed-loop PID control using optional Hall sensor for direct stir bar RPM.
    - Automatic fallback to Fan Tachometer PID if the stir bar sensor is absent.
- **Safety First:**
    - **Linear Ramping:** Smooth acceleration/deceleration to prevent magnetic decoupling.
    - **Physical Stall Detection:** Detects when the fan is physically obstructed.
    - **True Decoupling Detection:** Only possible with the optional Hall sensor; detects if the fan spins but the bar is stationary.
    - **Hardware E-STOP:** Optional High-side kill switch (GPIO 5) for complete power isolation.
- **Robust Integration:**
    - **Framed Telemetry:** Periodically broadcasts structured packets with CRC8 checksum via ESP-NOW and Serial1.
    - **Control Arbitration:** Explicit `REMOTE_LOCKED` mode for master-slave bioreactor control with local override safety.
- **Visual & Audible Feedback:**
    - **Heartbeat LED:** RGB LED pulses Green for activity and stays Red for safety stops.
    - **Audible Alarms:** Active buzzer for timer completion and decoupling alerts.

## 📁 Project Documentation

| File | Description |
| :--- | :--- |
| [HARDWARE.md](HARDWARE.md) | **Updated:** New pinout (Serial1 on 6/7, Hall on 4, E-STOP on 5). |
| [Bioreactor integration.md](Bioreactor%20integration.md) | Framed packet structures (CRC8) and control arbitration logic. |
| [sensor_comparison.md](sensor_comparison.md) | Technical choice between ACS712 and INA219. |
| [software_refinement.md](software_refinement.md) | Advanced PID tuning and spectral analysis proposals. |
| [shopping_list_dz.md](shopping_list_dz.md) | Algeria-specific sourcing guide. |

## 🛠️ Getting Started

### 1. Requirements
- ESP32 Arduino Core 2.x or 3.x (both supported via conditional macros).
- `Adafruit_SSD1306`, `Adafruit_GFX`, `Adafruit_INA219`, `WiFi`, `esp_now`, `Preferences`.

### 2. Flashing
1. Ensure "USB CDC On Boot" is **ENABLED** in Arduino IDE for logging.
2. Select **ESP32-S3 Dev Module**.
3. Upload `stirrer_project/stirrer_project.ino`.

## 🤝 Ecosystem
Integrates with `Bioreacteur_ESP32_PI` and `NADAH_FLUOreacteur_ESP32`.
