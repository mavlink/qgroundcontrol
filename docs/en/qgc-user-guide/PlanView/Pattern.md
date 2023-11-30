# Pattern

The _Pattern tools_ (in the [PlanView](../PlanView/PlanView.md) _Plan Tools_) allow you to specify complex flight patterns using a simple graphical UI.
The available pattern tools depend on the vehicle (and support for the vehicle-type in the flight stack).

![Pattern Tool (Plan Tools)](../../../assets/plan/pattern/pattern_tool.jpg)

| Pattern                                                         | Description                                                                                                                                                                                        | Vehicles          |
| --------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------- |
| [Survey](../PlanView/pattern_survey.md)                         | Create a grid flight pattern over a polygonal area. <br />You can specify the polygon as well as the specifications for the grid and camera settings appropriate for creating geotagged images.    | All               |
| [Structure Scan](../PlanView/pattern_structure_scan_v2.md)      | Create a grid flight pattern that captures images over vertical surfaces (polygonal or circular). <br />These are typically used for the visual inspection or creation of 3D models of structures. | MultiCopter, VTOL |
| [Corridor Scan](../PlanView/pattern_corridor_scan.md)           | Create a flight pattern which follows a poly-line (for example, to survey a road).                                                                                                                 | All               |
| [Fixed Wing Landing](../PlanView/pattern_fixed_wing_landing.md) | Add a landing pattern for fixed wing vehicles to a mission.                                                                                                                                        | Fixed Wing        |
