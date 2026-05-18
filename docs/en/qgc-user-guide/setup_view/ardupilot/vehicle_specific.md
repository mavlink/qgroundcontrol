# Vehicle-Specific Setup (ArduPilot)

Some ArduPilot vehicle types display additional configuration pages that are only relevant to that vehicle class.

## Helicopter

Available on traditional helicopter frames (ArduPilot 4.0+):

- **Swashplate** — servo manual mode, swashplate type, collective direction, linearize servo, flybar mode, cyclic max, collective min/max/angles, zero-thrust collective, landing collective minimum
- **Rotor Speed Control (RSC)** — RSC mode, critical rotor speed, ramp/runup/cooldown times, setpoint, idle percentage, throttle curve (0/25/50/75/100%), governor settings (reference/range/RPM/torque/FF)

## Submarine

Available on ArduSub frames (ArduPilot 3.5+):

- **Sub Frame** — select the submarine frame configuration (BlueROV1, BlueROV2/Vectored, Vectored-6DOF, etc.) with a visual frame selector. Option to load vehicle default parameters after selection.
- **Lights** — configure light output channel assignments (`SERVOn_FUNCTION`), step size (`JS_LIGHTS_STEP`), and number of brightness steps (`JS_LIGHTS_STEPS`)

## Fixed Wing

Fixed-wing vehicles may show additional pages for airspeed sensor calibration when an airspeed sensor is detected.
