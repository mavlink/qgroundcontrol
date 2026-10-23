# PX4 Flight Modes Setup

The _Flight Modes_ section allows you to configure which [flight modes](http://docs.px4.io/main/en/getting_started/flight_modes.html) and other actions are triggered by particular switches/switch positions on your RC transmitter.

::: info
In order to set up flight modes you must already have

- [Configured your radio](../setup_view/radio.md) in order to set flight modes.
- [Setup the RC transmitter](flight_modes.md#transmitter-setup) (Flight Modes > Transmitter Setup)
  :::

To access this section, select the **Gear** icon (Vehicle Setup) in the top toolbar and then **Flight Modes** in the sidebar.

The page only shows settings the connected vehicle has, and only those that apply to its type.
It is designed for PX4 1.17 or later. Older firmware shows only the settings it has.
The choices, units and limits of each setting come from the vehicle's parameter metadata.
Changes are written to the vehicle immediately.

The top of the page shows the manual control source (`COM_RC_IN_MODE`).
When it does not use RC input, the mode channel and switches have no effect.
A warning also appears when two functions on the same RC channel can be on at the same time, such as a switch on a stick channel, or two switches with the same threshold.
Two switches can share a channel when their thresholds split it, for example one on when the switch is low and the other when it is high.

## Mode Selection

- **Selection method** — how the pilot selects one of six mode slots:
  - **Switch channel**: the position of one channel (`RC_MAP_FLTMODE`) selects the slot.
  - **Buttons**: each momentary button channel (`RC_MAP_FLTM_BTN`) selects a slot, lowest channel first. PX4 uses up to six.
  - **None**: modes are not selected from RC.

  PX4 ignores the buttons while a mode channel is set, so choosing a method clears the other.

- **Mode slots** — the flight mode for each slot (`COM_FLTMODE1` to `COM_FLTMODE6`).
  With a switch channel, each slot shows its PWM range and a live bar shows where the channel is.
  With buttons, each slot shows its button channel.
  The active slot is highlighted.
  External modes are shown by the name the vehicle reports for them.
  External mode slots with no mode registered are not offered, unless the slot is already selected, when it is marked _not running_.

  ::: info
  Position mode, return mode and mission mode [are recommended](https://docs.px4.io/main/en/config/flight_mode.html#what-flight-modes-and-switches-should-i-set).
  :::

- **Mode on boot** — the mode set at power-up (PX4 1.18 and later).

## Switches

Channels that trigger particular actions, such as _Return_, _Kill_ or landing gear.
Each switch shows its channel and threshold, and the live switch position once it is assigned.
The threshold is a fraction of the channel range: the switch is on above it, or below it when the threshold is negative.
_Flight termination_ cannot be undone until the vehicle reboots.

## Stick Gestures, Position Slow Mode and Pilot Takeover

- **Stick Gestures** — arming and disarming with the sticks, and the kill gesture.
- **Position Slow Mode** — speed limits for multicopter Position Slow mode, and the AUX knobs that scale a speed in flight from its minimum up to the normal manual limit. The speed limits apply when no knob is assigned.
- **Pilot Takeover** — the stick override that switches an automatic mode to Position when the pilot moves the sticks.

Select **Assigned** to hide switches and options that are not in use.

## Testing

1. Turn on your RC transmitter.
1. If a switch does not seem to respond, check on the [Radio](../setup_view/radio.md) page that it moves the expected channel.
1. Select each mode switch position or button in turn, and check that the expected slot is highlighted.
   Assigned switches show _On_ or _Off_ as they are moved.

## See Also

- [PX4 Flight Modes](https://docs.px4.io/main/en/flight_modes/)
