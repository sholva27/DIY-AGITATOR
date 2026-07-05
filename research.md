# Laboratory Stirrer Research & Science

This project is designed to bridge the gap between hobbyist DIY stirrers and professional laboratory equipment (like the IKA or VELP series).

## 1. Scientific Requirements
To be useful in a bioreactor or chemistry lab, a stirrer must meet these criteria:
- **Constant Speed:** Maintain RPM regardless of liquid volume or viscosity changes.
- **Safety:** Prevent "flyaway" stir-bars (decoupling) which can break glassware.
- **Thermal Isolation:** The motor (fan) should not heat the sample. ESP32-S3 allows for active thermal monitoring if a DS18B20 is added.

## 2. Component Selection Rationale

### why ESP32-S3?
- **Hardware PWM:** High-frequency (25kHz+) prevents audible hum in the motor.
- **Dual Core:** Isolates time-critical PID control from heavy OLED/WiFi tasks.
- **ESP-NOW:** Low-latency communication for multi-unit bioreactor arrays.

### Sensing Strategy
- **Primary Safety (Stall):** Handled by the Fan Tachometer.
- **Stir-Bar Stability:** Handled by a lateral Hall Effect sensor.
- **Power Health (Logging):** Handled by the INA219 (Optional). Note: Monitoring current does **not** reliably detect decoupling, as fan aero-drag is the dominant load.

## 3. Reference Material
Inspirations for the mechanical design and magnet alignment:
- **Open-Source Labware (University of Michigan):** Focus on 3D printed housing and magnetic coupling distances.
- **DIY Biohacker Communities:** Use of PC fans for budget-friendly bioreactors.
- **IKA Lab Stirrer Teardowns:** Showcasing the importance of the hall sensor for feedback loops.

## 4. Hardware Optimization (Advanced)
For professional use, consider these refinements:
- **Magnet Balance:** Use two small neodymium magnets (N52) balanced precisely on the fan hub.
- **Thermal Barrier:** A 3mm acrylic or glass plate between the fan and the flask to minimize heat transfer.
- **Lateral Hall Alignment:** Mount the Hall sensor on the **side** of the flask container to detect the bar's magnetic field without interference from the fan's motor.
