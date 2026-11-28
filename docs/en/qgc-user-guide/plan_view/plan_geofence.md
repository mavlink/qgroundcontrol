# Plan View - GeoFence

GeoFences allow you to create virtual regions within which the vehicle can fly, or in which it is _not allowed_ to fly.
You can also configure the action taken if you fly outside permitted areas.

::: info
If GeoFence is not supported by the connected vehicle firmware, the **Fence** tab shows a message saying so instead of the GeoFence options.
:::

## Create a GeoFence

To create a GeoFence:

1. Navigate to the Plan View
1. Select the **Fence** tab in the [Plan Editor Panel](plan_view.md#plan_editor_panel).
   The tab is enabled once the [planned home position](plan_view.md#planned_home) is set.

1. Insert a circular or polygon region by pressing the **+ Circle** or **+ Polygon** button at the top of the tab.
   A new region will be added to the map and to the list of fences below the buttons, and it becomes the selected fence.

:::tip
You can create multiple regions by pressing the buttons multiple times, allowing complex geofence definitions to be created.
:::

- Circular region:

  - Move the region by dragging the central dot on the map
  - Resize the circle by dragging the dot on the edge of the circle (or you can change the **Radius** value in the fence inspector).

- Polygon region:

  - Move the vertices by dragging the filled dots
  - Create new vertices by clicking the "unfilled" dots on the lines between the filled vertices.

1. By default new regions are created as _inclusion_ zones (vehicles must stay within the region).
   Change them to exclusion zones (where the vehicle can't travel) by selecting _Exclude_ in the fence inspector.

1. Check a circular region's _Center on home_ checkbox in the fence inspector to keep it centred on the vehicle home position.
   The checkbox is offered when connected to a vehicle whose firmware supports these circles.

The list also has a **Breach return point** entry.
Select it and press **Set at map center** to add an optional point (with altitude) that the vehicle will fly to if it breaches the fence.

## Edit/Delete a GeoFence

Select a GeoFence region to edit by clicking it in the list on the **Fence** tab.
You can then edit the region on the map as described in the previous section, and change its settings in the inspector below the list.

Delete the selected region by pressing the delete button (trash icon) in the inspector.

## Upload a GeoFence

The GeoFence is uploaded along with the rest of the plan using the **Upload** button in the [Plan Toolbar](plan_view.md#file).
