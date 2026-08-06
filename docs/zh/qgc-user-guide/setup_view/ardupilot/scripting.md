# Scripting (ArduPilot)

The Scripting page manages Lua scripts on ArduPilot vehicles that support onboard scripting.

## Enable Scripting

- **Enable Scripting** — toggle the `SCR_ENABLE` parameter to allow Lua scripts to run on the flight controller

## Script Management

Scripts are stored in the `/APM/scripts/` directory on the vehicle's SD card. The page provides:

- **Upload** — select a `.lua` file from your computer and upload it to the vehicle via MAVLink FTP
- **Script list** — shows all `.lua` files currently on the vehicle
  - **Download** — save a script from the vehicle to your local filesystem
  - **Delete** — remove a script from the vehicle (with confirmation dialog)

After uploading or deleting scripts, you may need to reboot the flight controller for changes to take effect.
