import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls
import QGroundControl.AutoPilotPlugins.APM

SetupPage {
    pageComponent: mainComponent

    property real _verticalSpacing: ScreenTools.defaultFontPixelHeight / 2
    property real _horizontalSpacing: ScreenTools.defaultFontPixelWidth * 2

    APMGimbalParams { id: gimbalParams; instance: 1 }

    Component {
        id: mainComponent

        Loader {
            sourceComponent: gimbalParams.instanceCount > 0 ? baseComponent : notSupportedComponent
        }
    }

    Component {
        id: baseComponent

        QGCGroupBox {
            contentSpacing: _verticalSpacing
            showDividers: false

            QGCGroupBoxTab {
                title: qsTr("Gimbal 1")

                APMGimbalInstance {
                    instance: 1
                    verticalSpacing: _verticalSpacing
                    horizontalSpacing: _horizontalSpacing
                }
            }

            QGCGroupBoxTab {
                title: qsTr("Gimbal 2")
                tabVisible: gimbalParams.instanceCount > 1

                Loader {
                    id: gimbal2Loader
                    sourceComponent: gimbalParams.instanceCount > 1 ? gimbal2Component : null
                }
            }
        }
    }

    Component {
        id: gimbal2Component

        APMGimbalInstance {
            instance: 2
            verticalSpacing: _verticalSpacing
            horizontalSpacing: _horizontalSpacing
        }
    }

    Component {
        id: notSupportedComponent

        QGCLabel {
            text: qsTr("Gimbal settings are not available for this firmware version.")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pointSize: ScreenTools.largeFontPointSize
            wrapMode: Text.WordWrap
            width: availableWidth
            height: availableHeight
        }
    }
}
