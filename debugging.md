# Debugging Guide

## 1. OLED Display not working
- **Check I2C Address:** Run an I2C scanner sketch. Default is usually `0x3C`.
- **Voltage:** Ensure the OLED is receiving 3.3V.
- **Wiring:** Swap SDA and SCL wires to test.

## 2. Fan doesn't change speed
- **PWM Compatibility:** Some fans require a 5V PWM signal. If 3.3V from the ESP32 doesn't work, use a simple transistor or level-shifter.
- **Power:** Verify the fan has a dedicated 12V supply.

## 3. RPM reads 0
- **Pull-up Resistor:** The Tachometer pin MUST have a 10k pull-up resistor to 3.3V.
- **Pin Interrupts:** Check if the correct GPIO (GPIO 13) is connected to the Fan's Tacho wire (usually green or white).

## 4. ESP32 Reboots / Crashes
- **ISR Safety:** Ensure NO `Serial.print` or `String` modifications happen inside `handleEncoder` or `handleTachoPulse`.
- **Power Spike:** Use a large capacitor (100uF - 470uF) across the 12V power rails to handle motor noise.

## 5. ESP-NOW Not Connecting
- **MAC Address:** Verify the receiver's MAC address is correctly entered in the sender's code.
- **WiFi Channel:** Both ESP32s must be on the same WiFi channel.
