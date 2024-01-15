# QGroundControl Quick Start

Getting _QGroundControl_ up and running is quick and easy:

1. [Download and install](../getting_started/download_and_install.md) the application.
1. Start _QGroundControl_.
1. Attach your vehicle to the ground station device via USB, through a telemetry radio, or over WiFi. _QGroundControl_ should detect your vehicle and connect to it automatically.

That's it! If the vehicle is ready to fly, _QGroundControl_ should display [Fly View](../fly_view/fly_view.md) as shown below (otherwise it will open [Setup View](../setup_view/setup_view.md)).

![](../../../assets/quickstart/fly_view_connected_vehicle.jpg)

A good way to become familiar with _QGroundControl_ is to start experimenting:

- Use the [toolbar](../toolbar/toolbar.md) to switch between the main views:
  - [Settings](../settings_view/settings_view.md): Configure the _QGroundControl_ application.
  - [Setup](../setup_view/setup_view.md): Configure and tune your vehicle.
  - [Plan](../plan_view/plan_view.md): Create autonomous missions.
  - [Fly](../fly_view/fly_view.md): Monitor your vehicle(s) while flying, including streaming video.
  - [Analyze] ** Description of Analyze view is missing **
- Click the _Status icons_ on the toolbar to find out the status of the connected vehicle.

While the UI is fairly intuitive, this documentation can also be referenced to find out more.

::: info
Make sure QGC has an internet connection when you connect a new vehicle. This will allow it to get the latest parameter and other metadata for the vehicle, along with [translations](../settings_view/general.md#miscellaneous).
:::
