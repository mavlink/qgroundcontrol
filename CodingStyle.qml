/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// This is an example Qml file which is used to describe the QGroundControl coding style.
/// In general almost everything in here has some coding style meaning including order of
/// code. Not all style choices are explained. If there is any confusion please ask
/// and we'll answer and update style as needed.
///
/// Qt6 Import Style: Use unversioned imports (QtQuick not QtQuick 2.15)
/// Import order: Qt modules, blank line, QGroundControl modules
Item {
    id: root  // Use descriptive id for root item, not underscore prefix

    // ===================================================================================
    // PROPERTY BINDING SECTION
    // Bind to item properties first, before property definitions
    // ===================================================================================

    width:  ScreenTools.defaultFontPixelHeight * 10 // No hardcoded sizing. All sizing must be relative to a ScreenTools font size
    height: ScreenTools.defaultFontPixelHeight * 20

    // ===================================================================================
    // PUBLIC PROPERTIES SECTION
    // Property definitions available to consumers of this Qml Item come first
    // Group by type for readability
    // ===================================================================================

    property int myIntProperty: 10
    property real myRealProperty: 20.0
    property string myStringProperty: "example"
    property bool myBoolProperty: false

    // ===================================================================================
    // PRIVATE PROPERTIES SECTION
    // Property definitions which are internal to the item are prepended with an underscore
    // to signal private and come second. Use readonly appropriately to increase binding performance.
    // ===================================================================================

    readonly property real _rectWidth: ScreenTools.defaultFontPixelWidth * 10
    readonly property real _rectHeight: ScreenTools.defaultFontPixelWidth * 10
    readonly property bool _debugMode: false
    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    // ===================================================================================
    // SIGNALS SECTION
    // Define custom signals for communication
    // ===================================================================================

    signal buttonClicked()
    signal valueChanged(int newValue)

    // ===================================================================================
    // FUNCTIONS SECTION
    // Function definitions come after properties
    // ===================================================================================

    function myFunction() {
        console.log("myFunction was called")
    }

    function _privateFunction() {
        // Private functions prefixed with underscore
        if (_activeVehicle) {
            console.log("Active vehicle:", _activeVehicle.id)
        }
    }

    function calculateSomething(param1, param2) {
        // Always validate parameters
        if (!param1 || !param2) {
            console.warn("Invalid parameters")
            return 0
        }
        return param1 * param2
    }

    // ===================================================================================
    // VISUAL COMPONENTS SECTION
    // Child items come last
    // ===================================================================================

    // Use QGCPalette for all color theming - no hardcoded colors
    QGCPalette {
        id:                 qgcPal  // Note how id does not use an underscore
        colorGroupEnabled:  enabled
    }

    // Use ColumnLayout or RowLayout for better organization when appropriate
    ColumnLayout {
        anchors.fill:   parent
        spacing:        ScreenTools.defaultFontPixelHeight * 0.5

        // You should always use the QGC provided variants of base controls since they automatically support
        // our theming and font support (QGCButton, QGCLabel, QGCTextField, QGCCheckBox, etc.)
        QGCButton {
            // Note how there is no id: specified for this control. Only add id: if it is needed.
            Layout.fillWidth:   true
            text:               qsTr("Click Me")  // Use qsTr() for all user-visible strings

            onClicked: {
                myFunction()
                buttonClicked()  // Emit signal
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            text:               qsTr("Example Label: %1").arg(myIntProperty)
            wrapMode:           Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            color:              qgcPal.window   // Use QGC palette colors for everything, no hardcoded colors
            border.color:       qgcPal.text
            border.width:       1

            QGCLabel {
                anchors.centerIn:   parent
                text:               _debugMode ? qsTr("Debug Mode") : qsTr("Normal Mode")
                color:              qgcPal.text
            }
        }

        // Example: Conditional visibility
        QGCButton {
            Layout.fillWidth:   true
            text:               qsTr("Vehicle Action")
            visible:            _activeVehicle !== null  // Defensive: always check for null vehicle
            enabled:            _activeVehicle ? _activeVehicle.armed : false

            onClicked: {
                if (_activeVehicle) {  // Always null-check before use
                    _privateFunction()
                }
            }
        }
    }

    // ===================================================================================
    // CONNECTIONS SECTION
    // Connections objects come after visual components
    // ===================================================================================

    Connections {
        target: QGroundControl.multiVehicleManager

        function onActiveVehicleChanged(vehicle) {
            if (vehicle) {
                console.log("Active vehicle changed:", vehicle.id)
            }
        }
    }

    // ===================================================================================
    // COMPONENT.ONCOMPLETED SECTION
    // Initialization code comes at the end
    // ===================================================================================

    Component.onCompleted: {
        console.log("CodingStyle QML component loaded")
        _privateFunction()
    }

    // For scoped blocks which are long include a comment so you can tell what the brace is matching.
    // This is very handy when the top level brace scrolls off the screen. The end-brace comment in this
    // specific file is only included as style info. This example code is not long enough to really need it.
} // Item - CodingStyle
