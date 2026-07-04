# Bioreactor Integration Guide

## 1. Data Integrity (CRC8 Protected)

All communication over ESP-NOW and Serial1 uses framed packets with a CRC8 checksum.

### Master -> Stirrer (Command)
```cpp
#pragma pack(push, 1)
typedef struct {
  uint8_t start_byte;   // 0xAA
  uint8_t target_speed; // 0-100%
  uint8_t command;      // 1: Lock local encoder, 0: Unlock
  uint8_t crc8;
} stirrer_command_t;
#pragma pack(pop)
```

### Stirrer -> Master (Telemetry)
```cpp
#pragma pack(push, 1)
typedef struct {
  uint8_t start_byte;   // 0xBB
  uint8_t status;       // 0:IDLE, 1:LOCAL, 2:REM_LOCK, 3:SERIAL, 4:STOPPED, 5:STALL, 6:DECOUPLED
  uint8_t setpoint;     // Current target %
  int16_t bar_rpm;      // Real bar RPM
  int16_t fan_rpm;      // Internal fan RPM
  uint8_t crc8;
} stirrer_telemetry_t;
#pragma pack(pop)
```

## 2. Master Heartbeat
In `REMOTE_LOCKED` mode, the stirrer expects a command/heartbeat every 5 seconds. If communication is lost, it will perform a safety shutdown.
