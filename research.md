# Research: DIY Magnetic Stirrer

## 1. Fan Specifications (Intel/AMD Standard)
- **PWM Frequency:** 25 kHz is the target frequency. Frequencies significantly higher or lower can cause audible noise (humming) or failure to start.
- **Control Type:** Pulse Width Modulation on the 4th wire. 5V or 3.3V logic levels are generally acceptable.
- **Tachometer:** Typically 2 pulses per revolution (Open-collector).

## 2. Magnetic Coupling
- **Magnets:** Neodymium (N52) magnets are recommended for high torque.
- **Alignment:** Magnets must be perfectly centered on the fan hub to prevent vibration and bearing wear.
- **Distance:** The gap between the stirrer bar and the magnets should be minimized (3-5mm) for best coupling.

## 3. ESP32-S3 Pin Selection
- **I2C:** Default pins are GPIO 8 (SDA) and 9 (SCL).
- **USB-CDC:** ESP32-S3 has native USB, but hardware UART (Serial1) is better for inter-device communication like the Bioreactor link.
- **LEDC Peripheral:** Provides 8 independent PWM channels with hardware-controlled frequency and duty cycle.

## 4. Communication Protocols
- **ESP-NOW:** Peer-to-peer protocol by Espressif. No router required. Ideal for local bioreactor modules.
- **I2C vs UART:** UART is more robust over longer cable lengths (up to 1-2 meters) compared to I2C (centimeters).

## 5. Acceleration Ramping (Soft Start)
- **Problem:** Sudden jumps in fan speed (e.g. 0% to 100%) create high inertia torque. Since the magnetic coupling between the fan and the stirrer bar has a limited force, the bar can "break free" and sit vibrating in the center.
- **Solution:** Implementing a linear ramp (Soft Start). By gradually increasing the PWM duty cycle (e.g. 0.5% per 50ms), the stirrer bar can follow the magnetic field without decoupling.
