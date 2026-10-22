# Battery (PX4)

The Battery page configures how PX4 measures each battery and estimates its charge.
It brings the battery settings that PX4 spreads across the battery, power monitor and DroneCAN parameter groups into one place.
It is designed for PX4 1.17 or later. Older firmware shows only the settings it has.

The page only shows settings the connected vehicle has.
PX4 only sends some settings once a module that uses them is running, such as a power monitor's shunt resistance or a second battery on a board with one power input.
The page asks the vehicle for these when it opens and shows _Loading settings from the vehicle..._ until they arrive.
Settings the vehicle doesn't answer are not asked for again until it reconnects.
The choices, units and limits of each setting come from the vehicle's parameter metadata.
ESC calibration is still on the [Power](../power.md) page.

## Battery Rows

PX4 supports up to three batteries. Each row shows:

- **Battery** — the battery number, the parameters in use and a description of the selected source.
  The dot next to the name is green when the battery is monitored, and hollow when its source is _Disabled_.
  For a _Power Module_ or _ESCs_ battery, its live voltage, charge and current are shown under the parameters once the vehicle reports it.
  An _External_ battery reports its own ID rather than the battery number, so its live values are listed under **Other Reported Batteries** instead.
- **Cells** — one captioned cell per setting.
  Only the settings the selected source uses are shown:
  - _Power Module_ (an analog power module or an I2C power monitor) uses the cell count, voltages, capacity and internal resistance, and the analog calibration: voltage divider, amps per volt, ADC channels and filters.
  - _ESCs_ uses the cell count, voltages, capacity and internal resistance from ESC telemetry. PX4 only reads ESC telemetry into battery 1.
  - _External_ uses data reported over MAVLink, DroneCAN or by a driver, which estimates the charge itself.
    With the DroneCAN battery data set to _Filter data_, PX4 runs its own estimate and the cell, voltage and capacity settings are shown.

Settings that only take effect after the vehicle reboots are marked _Applies after reboot_.
Once one is changed from the value it had when the page opened, the mark becomes a _Reboot required_ warning.
The page shows these marks instead of the reboot notice that QGroundControl shows elsewhere. Reboot from the main status indicator in the toolbar.

The voltage divider and amps per volt cells have a **Calculate** button.
Measure the battery voltage or current with an external meter and enter it to calculate the value.
The button is available while the vehicle is reporting that battery and the value is set.
It scales the current value, so it can't start from -1, which uses the board's default.

## All Batteries

- **Charge levels** — the remaining charge at which every battery is reported low, critical and emergency.
  What the vehicle does at each level is set on the [Failsafes](failsafes.md) page.
- **Flight time estimate** — the expected current in flight, used for the remaining flight time until PX4 has measured the average. It needs the battery capacity.
- **Analog current offset** — the current sensor offset for analog power modules.

## Other Reported Batteries

Lists every battery the vehicle reports that isn't linked to a battery row above, with its ID and live voltage, charge and current.
DroneCAN batteries report their node ID minus one, and MAVLink batteries report 255.

## Power Monitors

The INA226, INA228, INA238 and INA220 I2C power monitors, the VOXL power module and DroneCAN batteries each have a row with their driver and shunt settings.
These report as a _Power Module_ battery source, except DroneCAN which uses _External_.

Select **In Use** to hide disabled batteries and power monitors.
The page opens on **In Use** when any battery or power monitor is in use, once the settings have loaded.
Changes are written to the vehicle immediately.
