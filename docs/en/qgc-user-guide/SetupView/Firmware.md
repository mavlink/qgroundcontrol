# Loading Firmware

*QGroundControl* **desktop** versions can install [PX4 Pro](http://px4.io/) or [ArduPilot](http://ardupilot.com) firmware onto Pixhawk-family flight-controller boards. By default QGC will install the current stable version of the selected autopilot, but you can also choose to install beta builds, daily builds, or custom firmware files. 

*QGroundControl* can also install the firmware for SiK Radios and PX4 Flow devices.

> **Caution** Loading Firmware is currently not available on tablet or phone versions of *QGroundControl*.


## Connect Device for Firmware Update

> **Caution** **Before you start installing Firmware** all USB connections to your vehicle must be *disconnected* (both direct or through a telemetry radio). The vehicle must *not be* powered by a battery.

1. First select the **Gear** icon (*Vehicle Setup*) in the top toolbar and then **Firmware** in the sidebar. 

  ![Firmware disconnected](../../../assets/setup/firmware/firmware_disconnected.jpg)

1. Connect your device (Pixhawk, SiK Radio, PX4 Flow) directly to your computer via USB. 

   > **Note** Connect directly to a powered USB port on your machine (do not connect through a USB hub).


## Select Firmware to Load

Once the device is connected you can choose which firmware to load (*QGroundControl* presents sensible options based on the connected hardware). 

1. For a Pixhawk-compatible board choose either **PX4 Flight Stack vX.X.X Stable Release** or **ArduPilot Flight Stack** radio buttons to download the *current stable release*.

   ![Select PX4](../../../assets/setup/firmware/firmware_select_default_px4.jpg)

   If you select *ArduPilot* you will also have to choose the specific firmware and the type of vehicle (as shown below).
   
   ![Select ArduPilot](../../../assets/setup/firmware/firmware_selection_ardupilot.jpg)
1. Check **Advanced settings** to select specific developer releases or install firmware from your local file system.

   ![ArduPilot - Advanced Settings](../../../assets/setup/firmware/firmware_selection_advanced_settings.jpg)
   

## Update the firmware

1. Click the **OK** button to start the update.

   The firmware will then proceed through a number of upgrade steps (downloading new firmware, erasing old firmware etc.). 
   Each step is printed to the screen and overall progress is displayed on a progress bar.

   ![Firmware Upgrade Complete](../../../assets/setup/firmware/firmware_upgrade_complete.jpg)

Once the firmware has finished loading the device/vehicle will reboot and reconnect. 
Next you will need to configure the [airframe](../SetupView/Airframe.md) (and then sensors, radio, etc.)

