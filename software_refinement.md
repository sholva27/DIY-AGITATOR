# Software Refinement & Performance Tuning

This guide provides advanced tips for developers looking to push the DIY Stirrer firmware to professional performance levels.

## 1. INA219 Configuration for Low Currents
The default settings of most INA219 libraries are tuned for high-current applications (3A+). For our stirrer (150-500mA), use these settings:

- **PGA (Programmable Gain Amplifier):** Set to **±40mV** (Range 8). This utilizes the full ADC dynamic range for the small voltage drop across the 0.1Ω shunt.
- **Bus Voltage Range:** 16V (instead of 32V) for better resolution on the 12V rail.
- **Averaging:** Enable 12-bit, 8-sample averaging (532µs conversion time) to filter out residual motor noise.

```cpp
// Example using Adafruit_INA219
ina219.setCalibration_16V_400mA(); // Custom calibration function
```

## 2. PID Tuning (Sub-100 RPM)
Low-speed stability is difficult due to static friction (stiction).
- **Integral Anti-Windup:** Clamp the `integral` value to prevent it from skyrocketing when the bar is stuck.
- **Feed-Forward:** Add a base PWM value (e.g., `PWM = Base + PID_Output`) where `Base` is the minimum PWM required to overcome friction, found during auto-calibration.

## 3. Handling PWM Aliasing
If you are NOT using a 4-wire fan and are instead haching the 12V line with a MOSFET:
- The INA219 will see 0V when the MOSFET is off and 12V when it is on.
- **Solution:** Perform "Synchronized Sampling" where the current is only read when the PWM pin is HIGH, or add a 470µF-1000µF capacitor across the fan leads to smooth the current draw into a DC value.

## 4. UI Responsiveness
- **OLED Throttling:** Ensure `display.display()` is called no more than 5-10 times per second. Higher rates waste I2C bandwidth and can jitter the PID loop (even on a separate core due to bus contention).
- **Encoder Polling:** Use interrupts (as implemented in `stirrer_project.ino`) rather than polling for instant feedback.

## 5. Security & Safety
- **ESP-NOW Pairing:** Implement a "pairing mode" where the stirrer only accepts commands from a Master MAC address stored in NVS (Preferences).
- **Serial CRC:** Always verify the CRC8 of incoming serial packets. Malformed packets can cause sudden RPM jumps.
