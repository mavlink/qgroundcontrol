# QGroundControl Quick Start

Getting *QGroundControl* up and running is quick and easy:

1. [Download and install](../getting_started/download_and_install.md) the application.
1. Start *QGroundControl*. 
1. Attach your vehicle to the ground station device via USB, through a telemetry radio, or over WiFi. *QGroundControl* should detect your vehicle and connect to it automatically.

That's it! If the vehicle is ready to fly,  *QGroundControl* should display [Fly View](../FlyView/FlyView.md) as shown below (otherwise it will open [Setup View](../SetupView/SetupView.md)).

![](../../../assets/quickstart/fly_view_connected_vehicle.jpg)

A good way to become familiar with *QGroundControl* is to start experimenting:

- Use the [toolbar](../toolbar/toolbar.md) to switch between the main views:
  - [Settings](../SettingsView/SettingsView.md): Configure the *QGroundControl* application.
  - [Setup](../SetupView/SetupView.md): Configure and tune your vehicle.
  - [Plan](../PlanView/PlanView.md): Create autonomous missions.
  - [Fly](../FlyView/FlyView.md): Monitor your vehicle(s) while flying, including streaming video.
  - [Analyze] ** Description of Analyze view is missing **
- Click the *Status icons* on the toolbar to find out the status of the connected vehicle. 

While the UI is fairly intuitive, this documentation can also be referenced to find out more.

> **Note** Make sure QGC has an internet connection when you connect a new vehicle. This will allow it to get the latest parameter and other metadata for the vehicle, along with [translations](../SettingsView/General.md#miscellaneous).