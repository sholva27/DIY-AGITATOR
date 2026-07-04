# DIY Magnetic Stirrer for Lab & Bioreactor

Advanced **ESP32-S3** based magnetic stirrer node featuring closed-loop PID control and bioreactor telemetry.

## 🚀 Professional Features

- **PID Closed-Loop Control:** Target real RPM values, with stir bar Hall-sensor feedback and automatic Fan Tachometer fallback.
- **Bi-Core Multi-Tasking:** FreeRTOS powered architecture ensures the 100Hz control loop never misses a pulse during UI updates.
- **True Safety Protocols:**
    - **Physical Stall Detection:** Detects obstructed fan blades.
    - **Bar Decoupling Detection:** Detects if the stir bar slips (requires optional side-mounted Hall sensor).
    - **Hardware Kill Switch:** GPIO 5 drives a high-side E-STOP to completely isolate power.
- **Robust Telemetry:** CRC8-protected framed packets over ESP-NOW and UART Serial1 (GPIO 6/7).
- **HMI Interface:** Non-blocking 0.96" OLED UI, Rotary Encoder with lock-arbitration, and RGB Heartbeat LED.

## 📁 Documentation Suite

- [HARDWARE.md](HARDWARE.md): Pinout and electrical engineering requirements.
- [Bioreactor integration.md](Bioreactor%20integration.md): Comm protocols for master controllers.
- [research.md](research.md): Fact-checked science on fan torque and sensor placement.
- [debugging.md](debugging.md): Signal verification and I2C troubleshooting.

## 🤝 Ecosystem
Designed for seamless integration with:
- **Bioreacteur_ESP32_PI**
- **NADAH_FLUOreacteur_ESP32**
