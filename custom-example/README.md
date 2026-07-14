# QGroundControl Ground Control Station

This directory is the sample described by the
[custom-build developer guide](https://dev.qgroundcontrol.com/en/custom_build/custom_build.html).
For repository-wide build and contribution workflows, see [tools/README.md](../tools/README.md) and
[CONTRIBUTING.md](../.github/CONTRIBUTING.md).

## Custom Build Example

To build this sample custom version:

1. Clean your build directory of any previous build.
2. Rename `custom-example` to `custom`.
3. `cd custom` and build QGC.

![Custom Build Screenshot](README.jpg)

This example demonstrates:

- **Off-the-shelf commercial vehicle** — most vehicle setup is hidden (pre-configured by the vendor),
  giving a simpler UI; the full experience stays available in **Advanced Mode**.
- **Custom branding** — images and color palette matching a corporate identity.
- **Custom interface** — e.g. a custom instrument widget replacing the standard QGC UI (see screenshot).
- **Overridden application settings** — hides options users shouldn't change and adjusts defaults.
- **Fully commented source** explaining what it does and why.
