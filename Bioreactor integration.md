# Bioreactor Integration: Stirrer, PI, and FLUOreacteur

## 1. Outgoing Telemetry (Node -> Master)

The Magnetic Stirrer node now provides real-time telemetry to the master controller.

### ESP-NOW Telemetry Structure
The node sends a `stirrer_telemetry_t` structure to the MAC address of the last sender.
```cpp
typedef struct {
  int actual_rpm;   // Current measured RPM
  int current_pwm;  // Current duty cycle (0-255)
  uint8_t status;   // 0:IDLE, 1:LOCAL, 2:REMOTE, 3:SERIAL, 4:STOPPED, 5:DECOUPLED, 6:TIMER_DONE
} stirrer_telemetry_t;
```

### Serial Telemetry (Headless / Wired)
Data is printed to `Serial1` (GPIO 5) every 1000ms:
`RPM:1200,PWM:150,STAT:REMOTE`

---

## 2. Remote Control (Master -> Node)

### ESP-NOW Command Structure
Send this structure to the stirrer's MAC address:
```cpp
typedef struct {
  int target_speed; // 0-100%
  bool remote_lock; // Set to TRUE to disable the local rotary encoder
} stirrer_command_t;
```

---

## 3. Control Priority & Safety

- **Arbitration:** If `remote_lock` is true, the stirrer ignores all rotary encoder rotations. The master has absolute priority.
- **Safety Feedback:** If the stirrer detects a stall (`DECOUPLED`), it will send status `5` in the telemetry. The master system should react by stopping pumps or heaters.
