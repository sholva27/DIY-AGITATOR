# Research: DIY Magnetic Stirrer

## 1. Fan Specifications
- **PWM Frequency:** 25 kHz is standard.
- **Tachometer:** Typically 2 pulses per revolution (Open-collector).
- **Torque & Coupling:** Fan motor torque depends on the motor design and static pressure rating, not the bearing type (Ball, Maglev, or Sleeve).

## 2. Magnetic Coupling & Sensing
- **Magnets:** Neodymium (N52) in a North-South alternating arrangement.
- **Sensing the Bar:**
    - **Hall Sensor:** Must be mounted **LATERALLY** (side-mounted) at the height of the stir bar. Placement directly under the container will only measure the strong drive magnets of the fan.
    - **Optical/IR:** A more robust alternative that is immune to magnetic crosstalk. An IR reflective sensor can detect the stir bar through clear container walls.

## 3. ESP32-S3 Specifics
- **Peripherals:** Uses LEDC for PWM. Dual-core allows separating the critical PID loop (Core 1) from the UI/Display (Core 0).
- **I2C:** Default SDA/SCL on 8/9.

## 4. Startup Dynamics
- **Kickstart:** PC fans often need a high initial duty cycle to overcome static friction and inertia. A 300ms burst at 100% is implemented.
- **Minimum Duty:** Every fan has a stall threshold (e.g., 15-20%). The firmware enforces a `MIN_PWM` to ensure reliable rotation.
