# Plan View - Rally Points

Rally Points are alternative landing or loiter locations.
They are typically used to provide a safer or more convenient (e.g. closer) destination than the home position in Return/RTL mode.

::: info
If Rally Points are not supported by the connected vehicle firmware, the **Rally** tab shows a message saying so instead of the Rally Point options.
:::

## Rally Point Usage

To create Rally Points:

1. Navigate to the Plan View
1. Select the **Rally** tab in the [Plan Editor Panel](plan_view.md#plan_editor_panel).
   The tab is enabled once the [planned home position](plan_view.md#planned_home) is set.
1. Click the map wherever you want rally points.
   - An **R** marker is added for each
   - The currently active marker has a different color (green) and its editor is shown in the inspector below the list of rally points.
1. Make any rally point active by selecting it on the map or in the list on the **Rally** tab:
   - Move the active rally point by either dragging it on the map or editing the position fields in the inspector.
   - Set its altitude and altitude frame in the inspector.
   - Delete a rally point by pressing the delete button (trash icon) in the inspector.

## Upload Rally Points

Rally points are uploaded along with the rest of the plan using the **Upload** button in the [Plan Toolbar](plan_view.md#file).
