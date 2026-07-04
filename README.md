# DIY Magnetic Stirrer for Lab & Bioreactor

A professional-grade magnetic stirrer powered by the ESP32-S3. This project features dual-core PID regulation, multiple sensing modes (Hall + Tacho), and robust communication for bioreactor integration.

## 🚀 Quick Start
1.  **Hardware:** Wire the ESP32-S3 as per `HARDWARE.md`.
2.  **Firmware:** Upload `stirrer_project/stirrer_project.ino` using the Arduino IDE (ESP32 by Espressif board package).
3.  **Libraries:**
    - Adafruit SSD1306 & GFX
    - Preferences (Included in ESP32 core)

## ✨ Core Features
-   **Precision RPM Control:** Uses PID logic to maintain speed under load.
-   **Dual Sensing:** Simultaneously monitors fan RPM and stir-bar RPM.
-   **Magnetic Decoupling Detection:** Alerts if the bar flies off the magnets.
-   **Bioreactor Integration:** ESP-NOW and Serial protocol (115200) with CRC8 verification.
-   **Persistence:** Remembers your last speed setting after power loss.
-   **Safety:** Integrated hardware kill-switch and stall protection.

## 📁 Documentation Index
-   **[Hardware Wiring](HARDWARE.md):** Pinouts, MOSFET circuits, and Hall placement.
-   **[Software Logic](software_logic.md):** Explanation of PID, FreeRTOS tasks, and safety state machines.
-   **[Integration Guide](Bioreactor%20integration.md):** Protocol details for connecting to a master controller.
-   **[Market/Research](research.md):** Sourcing tips for Algeria and component comparisons.
-   **[Future Roadmap](roadmap.md):** Plans for IoT, Web Dashboards, and Auto-Titration.

## 🛠 Advanced Tools
-   **[Bioreactor Client Example](bioreactor_client_example.ino):** Example code to control the stirrer remotely.
-   **[Refinement Guide](software_refinement.md):** Performance tuning and advanced calibration.
