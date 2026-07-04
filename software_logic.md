# Advanced Software Logic: PID & Multi-Sensing

This document details the internal logic used by the DIY Magnetic Stirrer to ensure precise control and safety.

## 1. Dual-Core Task Management
The ESP32-S3 firmware divides work between its two cores to ensure the control loop is never interrupted by heavy UI or communication tasks.

| Core | Task | Priority | Responsibility |
| :--- | :--- | :--- | :--- |
| **1** | `ControlTask` | 3 (High) | PID Loop (100Hz), ISR processing, PWM Generation, Safety checks. |
| **0** | `InterfaceTask` | 2 (Med) | OLED UI (5Hz), ESP-NOW & Serial comms, Encoder state machine. |

## 2. PID Control Strategy
The stirrer uses a PID (Proportional-Integral-Derivative) algorithm to maintain constant RPM regardless of liquid viscosity.

- **Setpoints:** User defines 0-100%, which is mapped to a target RPM range (e.g., 0 to 3000 RPM).
- **Process Variable (PV):**
    - If a Hall sensor detects the stir bar, **Bar RPM** is the PV.
    - If no bar is detected (or it decouples), **Fan RPM** (tachometer) is used as an fallback estimation.
- **Output:** PWM duty cycle (0-255) at 25kHz.

```cpp
float targetRPM = targetSetpoint * 30; // 0-3000 RPM range
float error = targetRPM - actualBarRPM;
integral += error * dt;
float output = (Kp * error) + (Ki * integral);
current_pwm_val = (targetSetpoint * 2.55) + output;
```

## 3. Hall Sensor Logic & Crosstalk Detection
A major challenge in magnetic stirrers is the Hall sensor accidentally reading the **drive magnets** inside the fan rather than the **stir bar**.

### The "Crosstalk" Filter
The firmware compares `actualBarRPM` with `actualFanRPM`.
- If `BarRPM == (FanRPM / PPR)`, the firmware assumes the Hall sensor is just reading the fan's internal magnets.
- In this state, the UI displays **[ESTIMATED]** to warn the user that true stir-bar speed is not being measured.
- **Solution:** Physically move the Hall sensor further away from the motor or add a small mu-metal shield.

## 4. Safety State Machine
The firmware monitors hardware health every 100ms:

1.  **FAN_STALL:** If PWM > 25% but Fan RPM < 100, the fan is physically blocked or the MOSFET failed. The system halts.
2.  **DECOUPLED:** If Fan RPM is high but Bar RPM < 50, the stir bar has lost magnetic lock. The system alerts the user and slows down to attempt re-coupling.
3.  **REMOTE_LOCKED:** If a bioreactor (Master) takes control via ESP-NOW, the local rotary encoder is disabled to prevent accidental tampering during a sensitive experiment.

## 5. Kickstart Routine
PC fans have significant inertia and static friction.
- Every time the stirrer starts, it applies **100% PWM for 400ms** (Kickstart) before handing control to the PID loop.
- This ensures the bar breaks static friction even in viscous liquids.

## 6. Communication Protocol
Frames are sent via Serial (6/7) and ESP-NOW.

**Telemetry Frame (15 bytes):**
`[0xBB] [Version] [Mode] [Setpoint] [BarRPM_H] [BarRPM_L] [FanRPM_H] [FanRPM_L] [mA_H] [mA_L] [Timer_4B] [CRC8]`

- **Heartbeat Safety:** If `REMOTE_LOCKED` mode is active and no command is received for 5 seconds, the stirrer enters a safe IDLE state.
