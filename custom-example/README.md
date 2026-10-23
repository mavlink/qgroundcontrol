# QGroundControl Ground Control Station

## Custom Build Example

To build this sample custom version:

1. Start from a clean build directory (`just clean`).
2. Rename `custom-example` to `custom` (the default `QGC_CUSTOM_DIR`), or keep the name and pass
   `-DQGC_CUSTOM_DIR=custom-example` when configuring.
3. Configure and build QGC from the repository root as usual.

![Custom Build Screenshot](README.jpg)

See the [QGC Dev Guide](https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/custom_build/custom_build.html)
for what a custom build is and how to create your own.

This example demonstrates:

- **Off-the-shelf commercial vehicle** — most vehicle setup is hidden (pre-configured by the vendor),
  giving a simpler UI; the full experience stays available in **Advanced Mode**.
- **Custom branding** — images and color palette matching a corporate identity.
- **Custom interface** — e.g. a custom instrument widget replacing the standard QGC UI (see screenshot).
- **Overridden application settings** — hides options users shouldn't change and adjusts defaults.
- **Fully commented source** explaining what it does and why.
