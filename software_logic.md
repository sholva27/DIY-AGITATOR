# Advanced Software Logic: PID & Multi-Sensing

This document details the internal logic used by the DIY Magnetic Stirrer to ensure precise control and safety.

## 1. Dual-Core Task Management
The ESP32-S3 firmware divides work between its two cores to ensure the control loop is never interrupted by heavy UI or communication tasks.

| Core | Task | Priority | Responsibility |
| :--- | :--- | :--- | :--- |
| **1** | `ControlTask` | 3 (High) | PID Loop (100Hz), ISR processing, PWM Generation, Safety checks. |
| **0** | `InterfaceTask` | 2 (Med) | OLED UI (5Hz), ESP-NOW & Serial comms, Encoder state machine. |

## 2. High-Precision RPM Measurement
Unlike standard hobbyist code that counts pulses over a fixed time (which has poor resolution at low speeds), this firmware uses **Period-Based Measurement**.

- **Mechanism:** Interrupts capture the exact `micros()` timestamp of each pulse.
- **Formula:** `RPM = 60,000,000 / Period_in_microseconds`.
- **Benefit:** Provides smooth, high-resolution feedback even at 60 RPM, enabling precise PID regulation.

## 3. PID Control Strategy
The stirrer uses a PID (Proportional-Integral-Derivative) algorithm to maintain constant RPM regardless of liquid viscosity.

- **Setpoints:** User defines 0-100%, which is mapped to a target RPM range (e.g., 0 to 3000 RPM).
- **Process Variable (PV):**
    - If a Hall sensor detects the stir bar, **Bar RPM** is the PV.
    - If no bar is detected (or it decouples), **Fan RPM** (tachometer) is used as an fallback estimation.
- **Robustness:** Includes **Integral Anti-Windup** to prevent overshoot when the motor is under high load or transitioning.

## 4. Hall Sensor Logic & Crosstalk Detection
A major challenge in magnetic stirrers is the Hall sensor accidentally reading the **drive magnets** inside the fan rather than the **stir bar**.

### The "Crosstalk" Filter
During the first 5 seconds of operation, the firmware compares `actualBarRPM` with `actualFanRPM`.
- If the values match perfectly (within a 5% margin), the firmware assumes the Hall sensor is just reading the fan's internal magnets and marks the sensor as **invalid**.
- This prevents "false safety" where the system thinks the bar is spinning just because the fan is spinning.

## 5. Safety State Machine
The firmware monitors hardware health every 100ms:

1.  **FAN_STALL:** If PWM > 25% but Fan RPM is 0, the fan is physically blocked. The system halts.
2.  **DECOUPLED:** If Fan RPM is high but Bar RPM is 0 (and the sensor was previously valid), the stir bar has lost magnetic lock. The system shuts down power to prevent glass breakage.
