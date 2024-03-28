# ArduSub Tuning

![ArduSub Tuning Page](../../../assets/setup/tuning/ardusub.jpg)

## Basic Tuning

This page allows changing the PID controller gains to better suit your vehicle and application. Changing these may help you get a snappier response for more precise movements, or a smoother response for recording cinematic footages. Adjust a parameter by moving the desired slider, or by clicking the increase/decrease buttons. There are three controllers that can be adjusted here:

- [**Attitude Controller Parameters**](https://www.ardusub.com/operators-manual/full-parameter-list.html#atc-parameters) are the parameters for the controller responsible for keeping the vehicle oriented as you want it, assuming your vehicle has ability (enough motors/DOF) to do so.

- **Position Controller Parameters** are the parameters for the controller responsible for positioning the vehicle at a point in 3D space. The **Z** parameters control how the depth control works (eg in [_Depth Hold_](https://www.ardusub.com/operators-manual/flight-modes.html#depth-hold) mode). The **XY** parameters affect how the vehicle controls the horizontal position in [_Position Enabled_](https://www.ardusub.com/operators-manual/flight-modes.html#position-enabled-modes) modes.

- **Waypoint Navigation Parameters** are the parameters for the controller responsible for following waypoints in **Auto** and **Guided** mode.

  ::: warning
  **Guided** and **Auto** modes are currently unsupported and some features are disabled in QGC.
  ::
