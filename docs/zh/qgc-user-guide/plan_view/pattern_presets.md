# Plan View - Pattern Presets

Allows you to save commonly used settings as a named preset.

:::info
Currently only supported by Survey. Support for other Patterns is in development.
:::

## Managing Presets

![Preset Combo](../../../assets/plan/pattern/pattern_preset_combo.jpg)

Pattern items have a new selection at the top which allows you to manage presets:

- **Custom (specify all settings)** This allows you to _not_ use a preset and specify all settings manually.
- **Save Settings As Preset** Saves the current settings as a named preset.
- **Delete Current Preset** Deletes the currently selected preset.
- **Presets:** Below this item will be listed the available presets for this pattern.

## Creating/Updating A Preset

![Preset Save](../../../assets/plan/pattern/pattern_preset_save.jpg)

When you select **Save Settings As Preset** you will be prompted for the preset name. To save new settings for an existing preset select **Save Settings As Preset** while a preset is currently selected.

You can also specify whether you want to save the currently selected camera in the preset. If you choose not to save the camera with the preset then the current camera will be used when loading the preset. You will also be able to change to a different camera when using the preset. Unless you fly your vehicle with different cameras at different times with the same preset you should select to save the camera in the preset.

## Viewing Preset Settings

If you want to view what the exact settings are for a Preset switch back to **Custom (specify all settings)** which will show you all the settings. Then you can switch back to using the named preset when done.

## Presets In A Plan File

The currently selected Preset is also saved in the Plan file such that when you load the Plan back the preset will once again be selected. Keep in mind that presets are specific to your version of QGroundControl. If you share a Plan file with a preset with another user, incorrect behavior may occur if that other user also has a preset of the same name but different settings.
