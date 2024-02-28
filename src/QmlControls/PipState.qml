/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

// Used to manage state for itesm using with QGCPipOveral
Item {
    id:     control
    state:  initState

    readonly property string initState:             "init"
    readonly property string pipState:              "pip"
    readonly property string fullState:             "full"
    readonly property string windowState:           "window"

    property var  pipView           // PipView control
    property bool isDark:   true    // true: Use dark overlay visuals

    signal windowAboutToOpen    // Catch this signal to do something special prior to the item transition to windowed mode
    signal windowAboutToClose   // Catch this signal to do special processing prior to the item transition back to pip mode

    property var _viewControl: control.parent

    states: [
        State {
            name: pipState

            AnchorChanges {
                target:         _viewControl
                anchors.top:    pipView._pipContentItem.top
                anchors.bottom: pipView._pipContentItem.bottom
                anchors.left:   pipView._pipContentItem.left
                anchors.right:  pipView._pipContentItem.right
            }

            ParentChange {
                target: _viewControl
                parent: pipView._pipContentItem
            }
        },
        State {
            name: fullState

            AnchorChanges {
                target:         _viewControl
                anchors.top:    pipView.parent.top
                anchors.bottom: pipView.parent.bottom
                anchors.left:   pipView.parent.left
                anchors.right:  pipView.parent.right
            }

            ParentChange {
                target: _viewControl
                parent: pipView.parent
            }
        },
        State {
            name: windowState

            AnchorChanges {
                target:         _viewControl
                anchors.top:    pipView._windowContentItem.top
                anchors.bottom: pipView._windowContentItem.bottom
                anchors.left:   pipView._windowContentItem.left
                anchors.right:  pipView._windowContentItem.right
            }

            ParentChange {
                target: _viewControl
                parent: pipView._windowContentItem
            }

            StateChangeScript {
                script: {
                    control.windowAboutToOpen()
                    pipView.showWindow()
                }
            }
        }
    ]
}
