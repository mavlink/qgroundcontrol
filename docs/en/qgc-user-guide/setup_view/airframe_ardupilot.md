# Airframe Setup (ArduPilot)

Airframe Setup is used to select the frame class and type that matches your vehicle

::: info
Airframe Setup is only available on _ArduCopter_ and _ArduSub_ vehicles (it is not shown for _ArduPilot_ Rover or Plane vehicles).
:::

## ArduCopter Airframe Setup

To select the airframe in Copter:

1. First select the **Gear** icon (Vehicle Setup) in the top toolbar and then **Airframe** in the sidebar.

   ![Airframe config](../../../assets/setup/airframe/arducopter.jpg)

1. Select the broad _Frame Class_ for your vehicle:

   ![Airframe type](../../../assets/setup/airframe/arducopter_class.jpg)

   ::: info
   You will need to reboot the vehicle for class changes to take effect.
   :::

1. Select the specific _Frame Type_ for your vehicle:

   ![Airframe type](../../../assets/setup/airframe/arducopter_type.jpg)

## ArduSub Frame Setup {#ardusub}

To select the frame type for Sub:

1. First select the **Gear** icon (Vehicle Setup) in the top toolbar and then **Frame** in the sidebar.
1. Select the frame type that matches your vehicle (selecting a frame applies the selection).
1. Make sure that all **green** thrusters have **clockwise** propellers and all **blue** thrusters have **counter-clockwise** propellers (or vice-versa).

   ![Select airframe type](../../../assets/setup/airframe_ardusub.jpg)

   - You can also click **Load Vehicle Default Parameters** to load default parameter set for ArduSub.

     ![Load vehicle params](../../../assets/setup/airframe_ardusub_parameters.jpg)
