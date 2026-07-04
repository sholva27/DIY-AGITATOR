# Bioreactor Integration: Stirrer, PI, and FLUOreacteur

## 1. Outgoing Telemetry (Node -> Master)

The Magnetic Stirrer node now provides real-time telemetry to the master controller.

### ESP-NOW Telemetry Structure
The node sends a `stirrer_telemetry_t` structure to the MAC address of the last sender.
```cpp
#pragma pack(push, 1)
typedef struct {
  uint8_t start_byte;   // 0xBB
  uint8_t len;
  uint8_t status;       // 0:IDLE, 1:LOCAL, 2:REM_LOCK, 3:SERIAL, 4:STOPPED, 5:STALL, 6:DECOUPLED, 7:DONE
  uint8_t setpoint;     // 0-100%
  int16_t bar_rpm;
  int16_t fan_rpm;
  uint16_t current_ma;
  uint32_t timer_rem;   // Remaining seconds
  uint8_t crc8;
} stirrer_telemetry_t;
#pragma pack(pop)
```

### Serial Telemetry (Headless / Wired)
Data is printed to `Serial1` (GPIO 7) every 1000ms:
`RPM:1200,PWM:150,STAT:REMOTE`

---

## 2. Remote Control (Master -> Node)

### ESP-NOW Command Structure
Send this structure to the stirrer's MAC address:
```cpp
#pragma pack(push, 1)
typedef struct {
  uint8_t start_byte;   // 0xAA
  uint8_t len;
  uint8_t target_speed; // 0-100%
  uint8_t command;      // 0: Release, 1: Acquire/Lock
  uint8_t crc8;
} stirrer_command_t;
#pragma pack(pop)
```

---

## 3. Control Priority & Safety

- **Arbitration:** If `command` is 1 (Acquire), the stirrer enters `REMOTE_LOCKED` mode and ignores all rotary encoder rotations. The master has absolute priority.
- **Heartbeat:** If the node is in `REMOTE_LOCKED` and receives no commands for >5 seconds, it will safe-stop.
- **Safety Feedback:** If the stirrer detects a stall (`FAN_STALLED`) or decoupling (`DECOUPLED`), it will update its status byte. The master system should react accordingly.
