# User Interface Controls

QGC provides a base set of controls for building user interface. In general they tend to be thin layers above the base QML Controls supported by Qt which respect the QGC color palette.

```
import QGroundControl.Controls 1.0
```

## Qt Controls

The following controls are QGC variants of standard Qt QML Controls. They provide the same functionality as the corresponding Qt controls except for the fact that they are drawn using the QGC palette.

- QGCButton
- QGCCheckBox
- QGCColoredImage
- QGCComboBox
- QGCFlickable
- QGCLabel
- QGCMovableItem
- QGCRadioButton
- QGCSlider
- QGCTextField

## QGC Controls

These custom controls are exclusive to QGC and are used to create standard UI elements.

- DropButton - RoundButton which drops out a panel of options when clicked. Example is Sync button in Plan view.
- ExclusiveGroupItem - Used as a base Item for custom controls which supports the QML ExclusiveGroup concept.
- QGCView - Base control for all top level views in the system. Provides support for FactPanels and displaying QGCViewDialogs and QGCViewMessages.
- QGCViewDialog - Dialog which pops out from the right side of a QGCView. You can specific the accept/reject buttons for the dialog as well as the dialog contents. Example usage is when you click on a parameter and it brings up the value editor dialog.
- QGCViewMessage - A simplified version of QGCViewDialog which allows you to specify buttons and a simple text message.
- QGCViewPanel - The main view contents inside of a QGCView.
- RoundButton - A round button control which uses an image as its inner contents.
- SetupPage - The base control for all Setup vehicle component pages. Provides a title, description and component page contents area.
- SelectableControl - Allows users to select from a list of controls to display. Right-click on Desktop or Press and Hold on Mobile will show selection UI. Example usage: Allows user to select between multiple Instrument display variants.
