# Ports (PX4)

The Ports page assigns functions such as MAVLink telemetry, GPS and RC input to the vehicle's serial ports, and to Ethernet on boards that have it.

PX4 stores this per function: each function has a parameter (for example `MAV_0_CONFIG` or `GPS_1_CONFIG`) that selects the port it runs on.
This page shows the same settings with one row per port.
It is only shown for PX4 builds that have serial port functions, so it does not appear for SITL.

## Port Rows

Each row shows:

- **Port** — the port name used by the flight controller, such as _TELEM 1_ or _GPS 1_.
- **Function** — what runs on the port. The functions normally used on that kind of port are listed first.
  Any function can run on any serial port, so the list also includes the others.
- **Baud** — the port's baud rate (`SER_<port>_BAUD`). Ethernet has no baud rate.
- **Options** — extra settings for the selected function: mode, rate, forwarding and flow control for MAVLink, and protocol for GPS.

Selecting a function for a serial port removes any other function from that port.
Several functions can use Ethernet at the same time, so the Ethernet row shows a checkbox for each one instead.

_Loading..._ is shown while QGroundControl reads a setting that the vehicle did not send in its parameter list.
PX4 only sends a port's baud rate and options once a function is using them.

If more than one function is assigned to the same serial port, the row shows a warning: only one of them will start.

PX4 always starts the RC Input Driver. Setting it to _Disabled_ does not stop it: it then uses the board's default RC port.
So the RC Input Driver stays on the _Radio Controller_ port until it is assigned to another port: that port has no _None_ choice meanwhile, and another function assigned there is shown as a conflict.

## Applying Changes

Changes are written to the vehicle immediately, but all of these settings only take effect after the vehicle reboots.
Changed rows are marked _Reboot required_, and a banner at the top of the page offers:

- **Reboot Vehicle** — reboot to apply the changes (not available while armed).
- **Revert Changes** — write back the values the settings had when the page was opened.

Both buttons are disabled until the vehicle has acknowledged every change, so a slow link can't apply only part of one.
