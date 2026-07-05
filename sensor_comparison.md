# Current Sensor Comparison: INA219 vs ACS712

For a laboratory magnetic stirrer, monitoring power consumption is a key metric for health and logging. This document evaluates the best sensor choice for the 12V DC rail.

## 1. The Verdict: INA219 (I2C Digital) wins over ACS712 (Hall Effect)

The **INA219** is the recommended choice for this project due to its digital precision, low noise floor, and immunity to the stirrer's magnets.

| Feature | ACS712 (Analog) | INA219 (I2C Digital) |
| :--- | :--- | :--- |
| **Principle** | Hall Effect (Magnetic) | Shunt Resistor (Voltage Drop) |
| **Magnetic Interference** | **Critical:** Pickups noise from the stirrer magnets. | **None:** Shunt-based, immune to magnets. |
| **Resolution** | Low (~66-100mV/A). Poor at <500mA. | High (12-bit). Excellent at <500mA. |
| **PWM Handling** | Aliases if not heavily filtered. | Requires averaging or 4-wire fan setup. |
| **Telemetry** | Current only (via ADC). | Current, Bus Voltage, and Power. |

---

## 2. Refining the Objective: What Current Sensing *Actually* Detects

It is a common misconception that current sensors can detect stir-bar decoupling. In a PC fan-based stirrer, the magnetic drag of the bar is negligible compared to the aerodynamic drag of the fan blades.

### Sensor vs. Failure Mapping
| Failure Mode | Detection Method | Notes |
| :--- | :--- | :--- |
| **Fan Stall (Blocked)** | **Fan Tachometer** | Free and instantaneous. Tacho goes to 0. |
| **Bar Decoupling** | **Lateral Hall Sensor** | Essential. Measures the bar directly. |
| **Power Supply Failure** | **INA219 (Bus Voltage)** | Detects 12V rail drop or fluctuation. |
| **Electronic Overload** | **INA219 (Current)** | Detects MOSFET shorts or motor winding failure. |
| **Long-term Logging** | **INA219 (Power/mA)** | Correlate power draw with liquid viscosity trends. |

---

## 3. Technical Implementation Details

### PWM Aliasing & Fan Choice
- **4-Wire Fans (Recommended):** The fan receives constant 12V; PWM is handled by an internal driver. Current draw is relatively continuous, making the INA219 readings stable.
- **3-Wire Fans (High-Side Chopping):** If you use a MOSFET to chop the 12V line, the INA219 will alias.
    - **Solution:** You must average the readings over multiple PWM cycles or place a large electrolytic capacitor (1000µF+) before the shunt to smooth the pulses.

### Low-Current Calibration (Sub-1A)
Standard INA219 modules (0.1Ω shunt) are often set to a ±320mV range (3.2A max). For a 150mA-500mA fan, this wastes resolution.
- **Optimization:** Configure the Programmable Gain Amplifier (PGA) to **±40mV** (Range 8). This increases resolution by 8x for low-power laboratory fans.
- **Hardware Mod:** For extreme precision, swap the 0.1Ω shunt for a **0.5Ω** shunt.

---

## 4. Alternatives: The "Pro" Choice
If you cannot find an INA219 or need even more precision:
1.  **INA226 (Superior Alternative):** 16-bit resolution (vs 12-bit) and lower offset voltage. It features a hardware **Alert Pin** that can trigger an ESP32 interrupt on overcurrent without CPU polling.
2.  **INA219 (Standard):** Listed in the `shopping_list_dz.md` due to high availability in Algeria (dzduino.com).
