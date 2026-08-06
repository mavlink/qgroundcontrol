# Plan View - GeoFence

GeoFences allow you to create virtual regions within which the vehicle can fly, or in which it is _not allowed_ to fly.
You can also configure the action taken if you fly outside permitted areas.

::: info
_QGroundControl_ will not display the GeoFence options if they are not supported by the connected vehicle firmware.
:::

## Create a GeoFence

To create a GeoFence:

1. Navigate to the Plan View
1. Select the **GeoFence** layer using the [Layer Switcher](plan_view.md#layer_switcher) in the top-right area of the map (or expand the **GeoFence** section in the [Plan Editor Panel](plan_view.md#plan_editor_panel))

1. Insert a circular or polygon region by pressing the **Circular Fence** or **Polygon Fence** button in the GeoFence section.
   A new region will be added to the map and to the associated list of fences below the buttons.

:::tip
You can create multiple regions by pressing the buttons multiple times, allowing complex geofence definitions to be created.
:::

- Circular region:


  - Move the region by dragging the central dot on the map
  - Resize the circle by dragging the dot on the edge of the circle (or you can change the radius value in the fence panel).

- Polygon region:


  - Move the vertices by dragging the filled dots
  - Create new vertices by clicking the "unfilled" dots on the lines between the filled vertices.

1. By default new regions are created as _inclusion_ zones (vehicles must stay within the region).
   Change them to exclusion zones (where the vehicle can't travel) by unchecking the associated _Inclusion_ checkbox in the fence panel.

Depending on the firmware, the GeoFence section may also show fence parameters (e.g. breach action) and a **Breach Return Point** that the vehicle will fly to if it breaches the fence.

## Edit/Delete a GeoFence

You can select a GeoFence region to edit by selecting its _Edit_ radio button in the GeoFence section.
You can then edit the region on the map as described in the previous section.

Regions can be deleted by pressing the associated **Del** button.

## Upload a GeoFence

The GeoFence is uploaded along with the rest of the plan using the **Upload** button in the [Plan Toolbar](plan_view.md#file).
