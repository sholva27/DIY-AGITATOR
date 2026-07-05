# Project Roadmap: Magnetic Stirrer & Bioreactor Module

## Phase 1: MVP (Completed)
- [x] Basic PWM Fan Control.
- [x] OLED Display integration.
- [x] Rotary Encoder for local control.
- [x] ESP-NOW & Serial communication.

## Phase 2: Software Refinement
- [ ] **PID Speed Control:** Maintain constant RPM regardless of liquid viscosity.
- [x] **Ramp Up/Down:** Prevent the magnetic bar from decoupling during sudden speed changes. (IMPLEMENTED)
- [ ] **Calibration Mode:** Map PWM % to real-world RPM values.

## Phase 3: Bioreactor Expansion
- [ ] **Temperature Integration:** Add DS18B20 sensor support for monitoring media temperature.
- [ ] **Web Dashboard:** Host a small web server on the ESP32 for monitoring via phone/browser.
- [ ] **Logging:** Log RPM and Time data to an SD card or via MQTT.

## Phase 4: Hardware Prototyping
- [ ] **Custom PCB:** Design an ESP32-S3 shield with built-in level shifters and buck converter.
- [ ] **3D Printed Housing:** Create a water-resistant enclosure for lab environments.

## Phase 5: Advanced Features (New Proposals)
- [ ] **PID Implementation:** Closed-loop speed control.
- [x] **Decoupling Protection:** Auto-detect and reset when the magnet slips. (IMPLEMENTED)
- [ ] **Sensor Fusion:** Display temperature (DS18B20) and current (INA219) data.
- [ ] **IoT Suite:** Web Dashboard, OTA Updates, and MQTT logging.
- [ ] **Vortex Lighting:** Integrated NeoPixel ring for visual status and lighting.
- [ ] **Predictive Maintenance:** Vibration analysis via accelerometer to detect bearing wear.
