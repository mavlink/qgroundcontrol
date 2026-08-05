# Plan View - Rally Points

Rally Points are alternative landing or loiter locations.
They are typically used to provide a safer or more convenient (e.g. closer) destination than the home position in Return/RTL mode.

::: info
_QGroundControl_ will not display the Rally Point options if they are not supported by the connected vehicle firmware.
:::

## Rally Point Usage

To create Rally Points:

1. Navigate to the Plan View
1. Select the **Rally Points** layer using the [Layer Switcher](plan_view.md#layer_switcher) in the top-right area of the map (or expand the **Rally Points** section in the [Plan Editor Panel](plan_view.md#plan_editor_panel))
1. Click the map wherever you want rally points.
   - An **R** marker is added for each
   - The currently active marker has a different color (green) and its editor is expanded in the _Rally Points_ section.
1. Make any rally point active by selecting it on the map or in the _Rally Points_ section:
   - Move the active rally point by either dragging it on the map or editing the position fields in its editor.
   - Delete a rally point by pressing the **X** delete button on its editor.

## Upload Rally Points

Rally points are uploaded along with the rest of the plan using the **Upload** button in the [Plan Toolbar](plan_view.md#file).
