/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtPositioning
import QtLocation
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.MultiVehicleManager
import QGroundControl.FlightMap
import QGroundControl.ShapeFileHelper
import QGroundControl.UTMSP

QGCFlickable {

    id:             _root
    contentHeight:  utmspEditorRect.height
    clip:           true

    property var    myGeoFenceController
    property var    mapPolygon
    property var    mapControl
    property var    flightMap
    property var    currentMissionItems
    property var    myActiveVehicle     :   QGroundControl.multiVehicleManager.activeVehicle
    property var    flightID
    property var    startTimeStamp
    property bool   submissionFlag
    property bool   triggerSubmitButton
    property bool   resetRegisterFlightPlan
    property var    endDay
    property var    endMonth
    property var    endYear

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 15)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2

    // Send parameters to PlanView qml
    signal responseSent(string response, bool responseFlag)                     // Send the flight blender response to PlanVeiw
    signal vehicleIDSent(string id)                                             // Send Vehcile ID to PlanView
    signal resetGeofencePolygonTriggered()                                      // Send FlightPlan Trigger Value to PlanView
    signal removeFlightPlanTriggered()

    // Set default Geofence Polygon
    function defaultPolygon(){
        var rect = Qt.rect(flightMap.centerViewport.x, flightMap.centerViewport.y, flightMap.centerViewport.width, flightMap.centerViewport.height)
        var topLeftCoord = flightMap.toCoordinate(Qt.point(rect.x, rect.y),false)
        var bottomRightCoord = flightMap.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height),false)
        myGeoFenceController.addInclusionPolygon(topLeftCoord,bottomRightCoord)
    }

    function loadEndDateTime(){
        var endHour = endhours.model[endhours.currentIndex]
        var startHour = starthours.model[starthours.currentIndex]
        var endDate = new Date();
        if (endHour < startHour){
            endDate.setDate(endDate.getDate() + 1);
        }
        endDay = endDate.getDate();
        endMonth = endDate.getMonth() + 1;
        endYear = endDate.getFullYear();
    }

    Rectangle {
        id:             utmspEditorRect
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         utmspFenceItems.y + utmspFenceItems.height + (_margin * 2)
        radius:         _radius
        color:          qgcPal.missionItemEditor

        QGCLabel {
            id:                 utmspFenceLabel
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.top:        parent.top
            text:               qsTr("UTM Service Editor")
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        }

        Rectangle {
            id:                 utmspFenceItems
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        utmspFenceLabel.bottom
            height:             fenceColumn.y + fenceColumn.height + (_margin * 2)
            color:              qgcPal.windowShadeDark
            radius:             _radius

            Column {
                id:                 fenceColumn
                anchors.margins:    _margin
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin

                // Login Interface
                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    Text{
                        text:           "Login / Registration"
                        color:          qgcPal.buttonText
                        font.pointSize: ScreenTools.smallFontPointSize
                    }
                }

                // LED Indicator with status
                Row{
                    spacing: _margin * 25

                    Switch {
                        id:      loginSwitch
                        text:    loginSwitch.checked ? qsTr("Enabled"):qsTr("Disabled")
                        visible: UTMSPStateStorage.loginState
                        indicator: Rectangle {
                            implicitWidth:  ScreenTools.defaultFontPixelWidth * 8
                            implicitHeight: ScreenTools.defaultFontPixelHeight * 1.44
                            x:              loginSwitch.leftPadding
                            y:              parent.height / 2 - height / 2
                            radius:         ScreenTools.defaultFontPixelHeight * 0.722
                            color:          loginSwitch.checked ? qgcPal.switchUTMSP : qgcPal.buttonText
                            border.color:   loginSwitch.checked ? qgcPal.switchUTMSP : qgcPal.colorGrey

                            Rectangle {
                                x:            loginSwitch.checked ? parent.width - width : 0
                                width:        ScreenTools.defaultFontPixelWidth * 4.33
                                height:       ScreenTools.defaultFontPixelHeight * 1.44
                                radius:       ScreenTools.defaultFontPixelHeight * 0.722
                                color:        loginSwitch.down ? qgcPal.colorGrey : qgcPal.buttonText
                                border.color: loginSwitch.checked ? (loginSwitch.down ? qgcPal.switchUTMSP : qgcPal.switchUTMSP) : qgcPal.colorGrey
                            }
                        }

                        contentItem: Text {
                            text:               loginSwitch.text
                            font:               loginSwitch.font
                            opacity:            enabled ? 1.0 : 0.3
                            color:              loginSwitch.checked ? qgcPal.colorGreen : qgcPal.buttonText
                            verticalAlignment:  Text.AlignVCenter
                            leftPadding:        loginSwitch.indicator.width + loginSwitch.spacing
                        }
                    }

                    // Logout button
                    QGCButton{
                        text:       qsTr("Logout")
                        width:      ScreenTools.defaultFontPixelWidth * 7.8
                        height:     ScreenTools.defaultFontPixelHeight * 2
                        visible:    !UTMSPStateStorage.loginState
                        onClicked:{
                            UTMSPStateStorage.loginState = !UTMSPStateStorage.loginState
                            loginSwitch.checked =!loginSwitch.checked
                            loading.visible = false
                            loginButton.opacity = 1
                            removeFlightPlanTriggered()
                            geoSwitch.enabled = true
                            UTMSPStateStorage.removeFlightPlanState

                            UTMSPStateStorage.indicatorIdleStatus = true
                            UTMSPStateStorage.indicatorApprovedStatus = false
                            UTMSPStateStorage.indicatorActivatedStatus = false
                            UTMSPStateStorage.indicatorDisplayStatus = false
                            UTMSPStateStorage.currentStateIndex = 0
                            UTMSPStateStorage.currentNotificationIndex = 5
                        }
                    }

                    Rectangle{
                        width:   ScreenTools.defaultFontPixelWidth * 0.1
                        height:  ScreenTools.defaultFontPixelHeight * 0.83
                        color:   qgcPal.windowShadeDark
                        visible: !UTMSPStateStorage.loginState
                    }

                    Row{
                        spacing: _margin * 5

                        QGCLabel{
                            id:     notifyText
                            x:      ScreenTools.defaultFontPixelWidth * 16.667
                            y:      ScreenTools.defaultFontPixelWidth * 0.833
                            text:   "Status:"
                            color:  qgcPal.buttonText
                        }

                        Image {
                            width:  ScreenTools.defaultFontPixelWidth * 5
                            height: ScreenTools.defaultFontPixelHeight * 1.667
                            source: UTMSPStateStorage.loginState? "qrc:/utmsp/red.png" : "qrc:/utmsp/green.png"
                            PropertyAnimation on opacity {
                                easing.type:    Easing.OutQuart
                                from:           0.5
                                to:             1
                                loops:          Animation.Infinite
                                running:        UTMSPStateStorage.loginState
                                alwaysRunToEnd: true
                                duration:       2000
                            }
                        }
                    }
                }

                Rectangle{
                    width:   ScreenTools.defaultFontPixelWidth * 0.5
                    height:  ScreenTools.defaultFontPixelHeight * 0.833
                    color:   qgcPal.windowShadeDark
                    visible: UTMSPStateStorage.loginState
                }

                Column{
                    spacing: _margin * 3.33
                    visible: UTMSPStateStorage.loginState

                    Column{
                        spacing:_margin * 1.667

                        QGCLabel        { text: qsTr("User ID") }
                        QGCTextField {
                            id:                     userName
                            width:                  ScreenTools.defaultFontPixelWidth * 50
                            height:                 ScreenTools.defaultFontPixelHeight * 1.667
                            enabled:                loginSwitch.checked
                            visible:                true
                            Layout.fillWidth:       true
                            Layout.minimumWidth:    _editFieldWidth
                            placeholderText:        "Enter your user ID"
                        }
                    }

                    Column{
                        spacing: _margin * 1.5

                        QGCLabel { text: qsTr("Password:") }
                        QGCTextField {
                            id:                     password
                            width:                  ScreenTools.defaultFontPixelWidth * 50
                            height:                 ScreenTools.defaultFontPixelHeight * 1.667
                            enabled:                loginSwitch.checked
                            visible:                true
                            echoMode:               TextInput.Password
                            Layout.fillWidth:       true
                            Layout.minimumWidth:    _editFieldWidth
                            placeholderText:        "Enter your password"
                        }
                    }

                    Row{
                        spacing: _margin * 30

                        Rectangle{
                            width:   ScreenTools.defaultFontPixelWidth * 0.5
                            height:  ScreenTools.defaultFontPixelHeight * 0.833
                            color:   qgcPal.windowShadeDark
                            visible: UTMSPStateStorage.loginState
                        }
                        QGCButton {
                            id:         loginButton
                            text:       qsTr("Login")
                            enabled:    loginSwitch.checked
                            width:      ScreenTools.defaultFontPixelWidth * 18
                            height:     ScreenTools.defaultFontPixelHeight * 1.667
                            x:          ScreenTools.defaultFontPixelWidth * 18
                            visible:    UTMSPStateStorage.loginState
                            AnimatedImage{
                                id:      loading
                                width:   ScreenTools.defaultFontPixelWidth * 4.5
                                height:  ScreenTools.defaultFontPixelHeight * 1.5
                                source:  "qrc:/utmsp/load.gif"
                                visible: false
                            }
                            onClicked:{
                                var validToken = QGroundControl.utmspManager.utmspAuthorization.requestOAuth2Client(userName.text,password.text)
                                if(validToken === true){
                                    loginButton.opacity = 0.5
                                    loading.visible = true
                                    delayTimer.interval = 2500
                                    delayTimer.repeat = false
                                    delayTimer.start()
                                    UTMSPStateStorage.indicatorDisplayStatus = true
                                    UTMSPStateStorage.currentNotificationIndex = 1
                                }
                                else{
                                    errorLogin.visible = true
                                }
                            }
                        }

                        Timer {
                            id:         delayTimer
                            running:    false
                            onTriggered: {
                                UTMSPStateStorage.loginState = !UTMSPStateStorage.loginState
                            }
                        }
                        Rectangle{
                            width:   ScreenTools.defaultFontPixelWidth * 0.5
                            height:  ScreenTools.defaultFontPixelHeight * 0.833
                            color:   qgcPal.windowShadeDark
                            visible: UTMSPStateStorage.loginState
                        }
                    }
                    Label{
                        id:                         errorLogin
                        text:                       "Incorrect Username or Password"
                        color:                      qgcPal.colorRed
                        visible:                    false
                        anchors.horizontalCenter:   parent.horizontalCenter
                    }

                    Row{
                        spacing: _margin * 23.33

                        Rectangle{
                            width:  ScreenTools.defaultFontPixelWidth * 2.5
                            height: ScreenTools.defaultFontPixelHeight * 0.278
                            color:  qgcPal.windowShadeDark
                        }
                        QGCLabel {
                            text:               qsTr("Forgot Your Password?")
                            Layout.alignment:   Qt.AlignHCenter
                            Layout.columnSpan:  3
                        }
                        Rectangle{
                            width:  ScreenTools.defaultFontPixelWidth * 2.5
                            height: ScreenTools.defaultFontPixelHeight * 0.278
                            color:  qgcPal.windowShadeDark
                        }
                    }

                    Row{
                        spacing:_margin * 23.33

                        Rectangle{
                            width:  ScreenTools.defaultFontPixelWidth * 2.5
                            height: ScreenTools.defaultFontPixelHeight * 0.278
                            color:  qgcPal.windowShadeDark
                        }
                        QGCLabel {
                            text:               qsTr("New User? Register Now") //TODO-->Will include register process
                            Layout.alignment:   Qt.AlignHCenter
                            Layout.columnSpan:  3
                            enabled:            true
                        }
                        Rectangle{
                            width:  ScreenTools.defaultFontPixelWidth * 2.5
                            height: ScreenTools.defaultFontPixelHeight * 0.278
                            color:  qgcPal.windowShadeDark
                        }
                    }
                }

                Rectangle{
                    width:  ScreenTools.defaultFontPixelWidth * 0.5
                    height: ScreenTools.defaultFontPixelHeight * 0.833
                    color:  qgcPal.windowShadeDark
                }

                Text{
                    text:       "Flight Plan Information"
                    font.bold:  true
                    color:      qgcPal.buttonText
                    visible:    !UTMSPStateStorage.loginState
                }

                Rectangle {
                    id:             flightinformation
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    width:          ScreenTools.defaultFontPixelWidth * 60
                    height:         ScreenTools.defaultFontPixelHeight * 0.1
                    color:          qgcPal.buttonText
                    visible:        !UTMSPStateStorage.loginState
                }

                // Geofence Interface
                SectionHeader {
                    id:             insertFence
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Insert Geofence")
                    checked:        false
                    visible:        !UTMSPStateStorage.loginState
                    PropertyAnimation on opacity {
                        easing.type:    Easing.OutQuart
                        from:           0.5
                        to:             1
                        loops:          Animation.Infinite
                        running:        !insertFence.checked
                        alwaysRunToEnd: true
                        duration:       2000
                    }
                }

                Column{
                    id:      trigger
                    spacing: _margin * 3.33
                }

                Column {
                    spacing: _margin * 3.33
                    visible: insertFence.checked && !UTMSPStateStorage.loginState
                    Switch {
                        id:   geoSwitch
                        text: geoSwitch.checked ? qsTr("Enabled") : qsTr("Disabled")
                        indicator: Rectangle {
                            implicitWidth:  ScreenTools.defaultFontPixelWidth * 8
                            implicitHeight: ScreenTools.defaultFontPixelHeight * 1.44
                            x:              geoSwitch.leftPadding
                            y:              parent.height / 2 - height / 2
                            radius:         ScreenTools.defaultFontPixelHeight * 0.722
                            color:          geoSwitch.checked ? qgcPal.switchUTMSP : qgcPal.buttonText
                            border.color:   geoSwitch.checked ? qgcPal.switchUTMSP : qgcPal.colorGrey
                            Rectangle {
                                x:            geoSwitch.checked ? parent.width - width : 0
                                width:        ScreenTools.defaultFontPixelWidth * 4.33
                                height:       ScreenTools.defaultFontPixelHeight * 1.44
                                radius:       ScreenTools.defaultFontPixelHeight * 0.722
                                color:        geoSwitch.down ? qgcPal.colorGrey : qgcPal.buttonText
                                border.color: geoSwitch.checked ? (geoSwitch.down ? qgcPal.switchUTMSP : qgcPal.switchUTMSP) : qgcPal.colorGrey
                            }
                        }
                        contentItem: Text {
                            text:               geoSwitch.text
                            font:               geoSwitch.font
                            opacity:            enabled ? 1.0 : 0.3
                            color:              geoSwitch.checked ? qgcPal.colorGreen : qgcPal.buttonText
                            verticalAlignment:  Text.AlignVCenter
                            leftPadding:        geoSwitch.indicator.width + geoSwitch.spacing
                        }
                        onCheckedChanged: {
                            if (checked) {
                                defaultPolygon()
                                if(resetRegisterFlightPlan === true){
                                    submitFlightPlan.visible = true
                                    deletePolygon.visible = true
                                    deleteFlightPlan.visible = false
                                }
                            }
                        }
                    }

                    Row{
                        Label{
                            text: qsTr("Min Altitude")
                            color: qgcPal.buttonText
                        }
                    }

                    Row {
                        spacing: _margin * 0.667

                        Row{
                            spacing: _margin * 0.667

                            Row{
                                spacing: _margin * 0.667

                                Rectangle{
                                    width:          ScreenTools.defaultFontPixelWidth * 15
                                    height:         ScreenTools.defaultFontPixelHeight * 1.667
                                    border.width:   ScreenTools.defaultFontPixelWidth * 0.167
                                    border.color:   qgcPal.buttonText
                                    color:          qgcPal.windowShadeDark
                                    Text {
                                        anchors.centerIn:   parent
                                        color:              qgcPal.buttonText
                                        text:               qsTr(minSlider.value.toFixed(0)+ "  m")
                                    }
                                }
                            }
                        }

                        Slider {
                            id:     minSlider
                            from:   0
                            value:  0
                            to:     10
                            background: Rectangle {
                                x:              minSlider.leftPadding
                                y:              minSlider.topPadding + minSlider.availableHeight / 2 - height / 2
                                implicitWidth:  ScreenTools.defaultFontPixelWidth * 30
                                implicitHeight: ScreenTools.defaultFontPixelHeight * 0.22
                                width:          minSlider.availableWidth
                                height:         implicitHeight
                                radius:         ScreenTools.defaultFontPixelHeight * 0.1111
                                color:          qgcPal.colorGrey
                                Rectangle {
                                    width:  minSlider.visualPosition * parent.width
                                    height: parent.height
                                    color:  qgcPal.sliderUTMSP
                                    radius: ScreenTools.defaultFontPixelHeight * 0.1111
                                }
                            }
                            handle: Rectangle {
                                x:              minSlider.leftPadding + minSlider.visualPosition * (minSlider.availableWidth - width)
                                y:              minSlider.topPadding + minSlider.availableHeight / 2 - height / 2
                                implicitWidth:  ScreenTools.defaultFontPixelWidth * 4.33
                                implicitHeight: ScreenTools.defaultFontPixelHeight * 1.44
                                radius:         ScreenTools.defaultFontPixelHeight * 0.722
                                color:          minSlider.pressed ? qgcPal.colorGrey : qgcPal.buttonText
                                border.color:   qgcPal.colorGrey
                            }
                        }
                    }

                    Row{
                        Label{
                            text:  qsTr("Max Altitude")
                            color: qgcPal.buttonText
                        }
                    }

                    Row{
                        spacing: _margin * 0.667

                        Row{
                            spacing: _margin * 0.667

                            Rectangle{
                                width:        ScreenTools.defaultFontPixelWidth * 15
                                height:       ScreenTools.defaultFontPixelHeight * 1.667
                                border.width: ScreenTools.defaultFontPixelWidth * 0.1667
                                border.color: qgcPal.buttonText
                                color:        qgcPal.windowShadeDark
                                Text {
                                    anchors.centerIn: parent
                                    color:            qgcPal.buttonText
                                    text:             qsTr(maxSlider.value.toFixed(0)+ "  m")
                                }
                            }
                        }
                        Slider {
                            id:     maxSlider
                            from:   0
                            value:  0
                            to:     120

                            background: Rectangle {
                                x:              maxSlider.leftPadding
                                y:              maxSlider.topPadding + maxSlider.availableHeight / 2 - height / 2
                                implicitWidth:  ScreenTools.defaultFontPixelWidth * 30
                                implicitHeight: ScreenTools.defaultFontPixelHeight * 0.22
                                width:          maxSlider.availableWidth
                                height:         implicitHeight
                                radius:         ScreenTools.defaultFontPixelHeight * 0.1111
                                color:          qgcPal.colorGrey
                                Rectangle {
                                    width:  maxSlider.visualPosition * parent.width
                                    height: parent.height
                                    color:  qgcPal.sliderUTMSP
                                    radius: ScreenTools.defaultFontPixelHeight * 0.1111
                                }
                            }
                            handle: Rectangle {
                                x:              maxSlider.leftPadding + maxSlider.visualPosition * (maxSlider.availableWidth - width)
                                y:              maxSlider.topPadding + maxSlider.availableHeight / 2 - height / 2
                                implicitWidth:  ScreenTools.defaultFontPixelWidth * 4.33
                                implicitHeight: ScreenTools.defaultFontPixelHeight * 1.44
                                radius:         ScreenTools.defaultFontPixelHeight * 0.7222
                                color:          maxSlider.pressed ? qgcPal.colorGrey : qgcPal.buttonText
                                border.color:   qgcPal.colorGrey
                            }
                        }
                    }

                    Row {
                        ListView {
                            id: deletePolygon
                            model: myGeoFenceController.polygons
                            delegate: QGCButton {
                                text: qsTr("Delete")
                                width: ScreenTools.defaultFontPixelWidth * 7.667
                                height: ScreenTools.defaultFontPixelHeight * 1.667
                                x: ScreenTools.defaultFontPixelWidth * 41
                                y: ScreenTools.defaultFontPixelHeight * 2
                                onClicked: {
                                    myGeoFenceController.deletePolygon(index)
                                    geoSwitch.checked =false
                                }
                            }
                        }
                    }
                }

                // Date Interface
                SectionHeader {
                    id:             dateandTime
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Date & Time")
                    checked:        false
                    visible:        !UTMSPStateStorage.loginState

                    PropertyAnimation on opacity {
                        easing.type:    Easing.OutQuart
                        from:           0.5
                        to:             1
                        loops:          Animation.Infinite
                        running:        !dateandTime.checked
                        alwaysRunToEnd: true
                        duration:       2000
                    }
                }

                TabBar {
                    visible:        dateandTime.checked && !UTMSPStateStorage.loginState
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    background: Rectangle {
                        color: qgcPal.buttonText
                    }

                    TabButton {
                        id:      dateButton
                        text:    qsTr("Date")
                        checked: !timeButton.checked

                        Image {
                            width:                          ScreenTools.defaultFontPixelWidth * 5
                            height:                         ScreenTools.defaultFontPixelHeight * 1.667
                            source:                         "qrc:/utmsp/date.svg"
                            anchors.horizontalCenter:       dateButton.horizontalCenter
                            anchors.verticalCenter:         dateButton.verticalCenter
                            anchors.horizontalCenterOffset: -40
                        }
                    }

                    TabButton {
                        id:      timeButton
                        text:    qsTr("Time")
                        checked: false

                        Image {
                            width:                          ScreenTools.defaultFontPixelWidth * 5
                            height:                         ScreenTools.defaultFontPixelHeight * 1.667
                            source:                         "qrc:/utmsp/time.svg"
                            anchors.horizontalCenter:       timeButton.horizontalCenter
                            anchors.verticalCenter:         timeButton.verticalCenter
                            anchors.horizontalCenterOffset: -40
                        }
                    }
                }

                Column{
                    spacing:  _margin * 3.33

                    Row{
                        visible:    dateButton.checked && dateandTime.checked && !UTMSPStateStorage.loginState
                        spacing:  _margin * 10

                        Rectangle{
                            width:  ScreenTools.defaultFontPixelWidth * 2.5
                            height: ScreenTools.defaultFontPixelHeight * 0.278
                            color:  qgcPal.windowShadeDark
                        }
                        Text{
                            text:  "Month"
                            color: qgcPal.buttonText
                        }
                        Text{
                            text: "      Day"
                            color: qgcPal.buttonText
                        }
                        Text{
                            text: "       Year"
                            color: qgcPal.buttonText
                        }
                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }
                    }

                    Row{
                        spacing:  _margin * 6.667
                        visible:  dateButton.checked && dateandTime.checked && !UTMSPStateStorage.loginState
                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }
                        Tumbler {
                            id: scrollMonth
                            model: {
                                var monthNames = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"]
                                return monthNames;
                            }
                            currentIndex: (new Date()).getMonth()
                            background: Item {
                                Rectangle {
                                    opacity:      scrollMonth.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }
                                Rectangle {
                                    opacity:        scrollMonth.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }
                            delegate: Text {
                                text:                modelData
                                font:                scrollMonth.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (scrollMonth.visibleItemCount / 2)
                            }
                            Rectangle {
                                anchors.horizontalCenter: scrollMonth.horizontalCenter
                                y:                        scrollMonth.height * 0.4
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                            Rectangle {
                                anchors.horizontalCenter: scrollMonth.horizontalCenter
                                y:                        scrollMonth.height * 0.6
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                        }

                        Tumbler {
                            id: scrollDate
                            model: {
                                var modelArray = []
                                for (var i = 1; i <= 31; i++) {
                                    modelArray.push(i.toString())
                                }
                                return modelArray;
                            }
                            currentIndex: (new Date()).getDate() - 1
                            background: Item {
                                Rectangle {
                                    opacity:      scrollDate.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }
                                Rectangle {
                                    opacity:        scrollDate.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }
                            delegate: Text {
                                text:                modelData
                                font:                scrollDate.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (scrollDate.visibleItemCount / 2)
                            }
                            Rectangle {
                                anchors.horizontalCenter: scrollDate.horizontalCenter
                                y:                        scrollDate.height * 0.4
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                            Rectangle {
                                anchors.horizontalCenter: scrollDate.horizontalCenter
                                y:                        scrollDate.height * 0.6
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                        }

                        Tumbler {
                            id: scrollYear
                            model: {
                                var modelArray = []
                                var currentYear = (new Date()).getFullYear()
                                for (var i = currentYear - 5; i <= currentYear + 10; i++) {
                                    modelArray.push(i.toString())
                                }
                                return modelArray;
                            }
                            currentIndex: 5
                            background: Item {
                                Rectangle {
                                    opacity:      scrollYear.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }
                                Rectangle {
                                    opacity:        scrollYear.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }
                            delegate: Text {
                                text:                modelData
                                font:                scrollYear.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (scrollYear.visibleItemCount / 2)
                            }
                            Rectangle {
                                anchors.horizontalCenter: scrollYear.horizontalCenter
                                y:                        scrollYear.height * 0.4
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                            Rectangle {
                                anchors.horizontalCenter: scrollYear.horizontalCenter
                                y:                        scrollYear.height * 0.6
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                        }

                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }
                    }
                }

                // Start and End Time Interface
                Column{
                    spacing:  _margin * 3.33

                    Row{
                        visible:    timeButton.checked && dateandTime.checked && !UTMSPStateStorage.loginState
                        spacing:    _margin * 10
                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }

                        Text{
                            text: " Hour"
                            color: qgcPal.buttonText
                        }

                        Text{
                            text: "    Minute"
                            color: qgcPal.buttonText
                        }

                        Text{
                            text: "  Second"
                            color: qgcPal.buttonText
                        }

                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }
                    }

                    Row{
                        spacing:   _margin * 6.667
                        visible:   timeButton.checked && startID.checked && dateandTime.checked && !UTMSPStateStorage.loginState
                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }

                        Tumbler {
                            id: starthours
                            model: {
                                var modelArray = []
                                for (var i = 0; i <= 23; i++) {
                                    modelArray.push(i.toString().padStart(2, '0'))
                                }
                                return modelArray;
                            }
                            currentIndex: (new Date()).getHours()

                            background: Item {
                                Rectangle {
                                    opacity:      starthours.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }

                                Rectangle {
                                    opacity:        starthours.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }

                            delegate: Text {
                                text:                modelData
                                font:                starthours.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (starthours.visibleItemCount / 2)
                            }

                            Rectangle {
                                anchors.horizontalCenter: starthours.horizontalCenter
                                y:                        starthours.height * 0.4
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }

                            Rectangle {
                                anchors.horizontalCenter: starthours.horizontalCenter
                                y:                        starthours.height * 0.6
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                        }

                        Tumbler {
                            id: startMinutes
                            model: {
                                var modelArray = []
                                for (var i = 0; i <= 59; i++) {
                                    modelArray.push(i.toString().padStart(2, '0'))
                                }
                                return modelArray;
                            }
                            currentIndex: (new Date()).getMinutes()+1
                            background: Item {
                                Rectangle {
                                    opacity:      startMinutes.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }
                                Rectangle {
                                    opacity:        startMinutes.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }
                            delegate: Text {
                                text:                modelData
                                font:                startMinutes.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (startMinutes.visibleItemCount / 2)
                            }
                            Rectangle {
                                anchors.horizontalCenter: startMinutes.horizontalCenter
                                y:                        startMinutes.height * 0.4
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                            Rectangle {
                                anchors.horizontalCenter: startMinutes.horizontalCenter
                                y:                        startMinutes.height * 0.6
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                        }

                        Tumbler {
                            id: startSeconds
                            model: {
                                var modelArray = []
                                for (var i = 0; i <= 59; i++) {
                                    modelArray.push(i.toString().padStart(2, '0'))
                                }
                                return modelArray;
                            }
                            currentIndex: (new Date()).getSeconds()
                            background: Item {
                                Rectangle {
                                    opacity:      startSeconds.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }

                                Rectangle {
                                    opacity:        startSeconds.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }
                            delegate: Text {
                                text:                modelData
                                font:                startSeconds.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (startSeconds.visibleItemCount / 2)
                            }
                            Rectangle {
                                anchors.horizontalCenter: startSeconds.horizontalCenter
                                y:                        startSeconds.height * 0.4
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                            Rectangle {
                                anchors.horizontalCenter: startSeconds.horizontalCenter
                                y:                        startSeconds.height * 0.6
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                        }
                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }
                    }

                    Row{
                        spacing:  _margin * 6.667
                        visible:  timeButton.checked && stopID.checked && dateandTime.checked && !UTMSPStateStorage.loginState
                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }

                        Tumbler {
                            id: endhours
                            model: {
                                var modelArray = []
                                for (var i = 0; i <= 23; i++) {
                                    modelArray.push(i.toString().padStart(2, '0'))
                                }
                                return modelArray;
                            }
                            currentIndex: (new Date()).getHours() +1
                            background: Item {
                                Rectangle {
                                    opacity:      endhours.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }
                                Rectangle {
                                    opacity:        endhours.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }
                            delegate: Text {
                                text:                modelData
                                font:                endhours.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (endhours.visibleItemCount / 2)
                            }
                            Rectangle {
                                anchors.horizontalCenter: endhours.horizontalCenter
                                y:      endhours.height * 0.4
                                width:  ScreenTools.defaultFontPixelWidth * 6.667
                                height: ScreenTools.defaultFontPixelHeight * 0.056
                                color:  qgcPal.colorGreen
                            }
                            Rectangle {
                                anchors.horizontalCenter: endhours.horizontalCenter
                                y:      endhours.height * 0.6
                                width:  ScreenTools.defaultFontPixelWidth * 6.667
                                height: ScreenTools.defaultFontPixelHeight * 0.056
                                color:  qgcPal.colorGreen
                            }
                        }

                        Tumbler {
                            id: endMinutes
                            model: {
                                var modelArray = []
                                for (var i = 0; i <= 59; i++) {
                                    modelArray.push(i.toString().padStart(2, '0'))
                                }
                                return modelArray;
                            }
                            currentIndex: (new Date()).getMinutes()
                            background: Item {
                                Rectangle {
                                    opacity:      endMinutes.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }
                                Rectangle {
                                    opacity:        endMinutes.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }
                            delegate: Text {
                                text:                modelData
                                font:                endMinutes.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (endMinutes.visibleItemCount / 2)
                            }
                            Rectangle {
                                anchors.horizontalCenter: endMinutes.horizontalCenter
                                y:                        endMinutes.height * 0.4
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                            Rectangle {
                                anchors.horizontalCenter: endMinutes.horizontalCenter
                                y:                        endMinutes.height * 0.6
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                        }

                        Tumbler {
                            id: endSeconds
                            model: {
                                var modelArray = []
                                for (var i = 0; i <= 59; i++) {
                                    modelArray.push(i.toString().padStart(2, '0'))
                                }
                                return modelArray;
                            }
                            currentIndex: (new Date()).getSeconds()
                            background: Item {
                                Rectangle {
                                    opacity:      endSeconds.enabled ? 0.2 : 0.1
                                    border.color: qgcPal.textFieldText
                                    width:        parent.width
                                    height:       ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.top:  parent.top
                                }
                                Rectangle {
                                    opacity:        endSeconds.enabled ? 0.2 : 0.1
                                    border.color:   qgcPal.textFieldText
                                    width:          parent.width
                                    height:         ScreenTools.defaultFontPixelHeight * 0.056
                                    anchors.bottom: parent.bottom
                                }
                            }
                            delegate: Text {
                                text:                modelData
                                font:                endSeconds.font
                                color:               qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                opacity:             1.0 - Math.abs(Tumbler.displacement) / (endSeconds.visibleItemCount / 2)
                            }
                            Rectangle {
                                anchors.horizontalCenter: endSeconds.horizontalCenter
                                y:                        endSeconds.height * 0.4
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                            Rectangle {
                                anchors.horizontalCenter: endSeconds.horizontalCenter
                                y:                        endSeconds.height * 0.6
                                width:                    ScreenTools.defaultFontPixelWidth * 6.667
                                height:                   ScreenTools.defaultFontPixelHeight * 0.056
                                color:                    qgcPal.colorGreen
                            }
                        }

                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 2.5
                            height:ScreenTools.defaultFontPixelHeight * 0.278
                            color: qgcPal.windowShadeDark
                        }
                    }

                    Row{
                        spacing:  _margin * 10
                        visible: !UTMSPStateStorage.loginState

                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 5
                            height:ScreenTools.defaultFontPixelHeight * 0.556
                            color: qgcPal.windowShadeDark
                        }

                        QGCCheckBox {
                            id:      startID
                            text:    qsTr("Start Time")
                            checked: true
                            enabled: !stopID.checked
                            visible: timeButton.checked
                        }

                        QGCCheckBox {
                            id:      stopID
                            text:    qsTr("End Time")
                            checked: false
                            visible: timeButton.checked
                        }

                        Rectangle{
                            width: ScreenTools.defaultFontPixelWidth * 5
                            height:ScreenTools.defaultFontPixelHeight * 0.556
                            color: qgcPal.windowShadeDark
                        }
                    }
                }

                // Mission Altitude
                Column{
                    visible:                !UTMSPStateStorage.loginState
                    QGCLabel        {
                        text: qsTr("Mission Altitude")
                    }

                    FactTextField {
                        width:                  ScreenTools.defaultFontPixelWidth * 50
                        height:                 ScreenTools.defaultFontPixelHeight * 1.667
                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 50
                        fact:                   QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
                    }
                }

                Rectangle {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    width:          ScreenTools.defaultFontPixelWidth * 60
                    height:         ScreenTools.defaultFontPixelHeight * 0.1
                    color:          qgcPal.buttonText
                }

                Connections {
                    target: myGeoFenceController
                    onPolygonBoundarySent: function(coords) {
                        QGroundControl.utmspManager.utmspVehicle.updatePolygonBoundary(coords)
                    }
                }

                QGCButton {
                    id:             submitFlightPlan
                    text:           qsTr("Register Flight Plan")
                    visible:        !UTMSPStateStorage.loginState && UTMSPStateStorage.registerButtonState && !UTMSPStateStorage.removeFlightPlanState
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    enabled:        geoSwitch.checked && triggerSubmitButton

                    onClicked: {
                        submissionTimer.interval = 2500
                        submissionTimer.repeat = false
                        submissionTimer.start()
                        loadEndDateTime()
                        var minAltitude   = minSlider.value.toFixed(0)
                        var maxAltitude   = maxSlider.value.toFixed(0)
                        var startDay      = scrollDate.model[scrollDate.currentIndex]
                        var startMonth    = scrollMonth.currentIndex + 1
                        var startYear     = scrollYear.model[scrollYear.currentIndex]
                        var startHour     = starthours.model[starthours.currentIndex]
                        var startMinute   = startMinutes.model[startMinutes.currentIndex]
                        var startSecond   = startSeconds.model[startSeconds.currentIndex]
                        var endHour       = endhours.model[endhours.currentIndex]
                        var endMinute     = endMinutes.model[endMinutes.currentIndex]
                        var endSecond     = endSeconds.model[endSeconds.currentIndex]
                        var startDateTime = startYear + "-" + startMonth + "-" + startDay + "T" + startHour + ":" + startMinute + ":" + startSecond+ "." + "000000" + "Z"
                        var endDateTime   = endYear + "-" + endMonth + "-" + endDay + "T" + endHour + ":" + endMinute + ":" + endSecond+ "." + "000000" + "Z"
                        var activateTD    = startYear + "-" + String(startMonth).padStart(2, '0') + "-" + String(startDay).padStart(2, '0') + "T" + String(startHour).padStart(2, '0') + ":" + String(startMinute).padStart(2, '0') + ":" + String(startSecond).padStart(2, '0') + "." + "000000" + "Z"
                        resetGeofencePolygonTriggered()
                        myGeoFenceController.loadFlightPlanData()
                        QGroundControl.utmspManager.utmspVehicle.updateStartDateTime(startDateTime.toString())
                        QGroundControl.utmspManager.utmspVehicle.updateEndDateTime(endDateTime.toString())
                        QGroundControl.utmspManager.utmspVehicle.updateMinAltitude(minAltitude)
                        QGroundControl.utmspManager.utmspVehicle.updateMaxAltitude(maxAltitude)
                        QGroundControl.utmspManager.utmspVehicle.triggerFlightAuthorization()
                        var responseFlightID = QGroundControl.utmspManager.utmspVehicle.responseFlightID
                        var responseFlag = QGroundControl.utmspManager.utmspVehicle.responseFlag
                        var responseJson = QGroundControl.utmspManager.utmspVehicle.responseJson
                        var serialNumber = QGroundControl.utmspManager.utmspVehicle.vehicleSerialNumber
                        vehicleIDSent(serialNumber)
                        flightID = responseFlightID
                        startTimeStamp = activateTD
                        submissionFlag = responseFlag
                        responseSent(responseJson,responseFlag)
                        UTMSPStateStorage.startTimeStamp = activateTD
                        UTMSPStateStorage.showActivationTab = responseFlag
                        UTMSPStateStorage.flightID = flightID
                        UTMSPStateStorage.serialNumber = serialNumber
                    }
                }

                Timer {
                    id:      submissionTimer
                    running: false
                    onTriggered: {
                        if(submissionFlag === true)
                        {
                            UTMSPStateStorage.enableMissionUploadButton = true
                            submitFlightPlan.visible = false
                            geoSwitch.checked = false
                            deletePolygon.visible = false
                            deleteFlightPlan.visible = true
                            geoSwitch.enabled = false
                            UTMSPStateStorage.indicatorIdleStatus = false
                            UTMSPStateStorage.indicatorApprovedStatus = true
                            UTMSPStateStorage.indicatorDisplayStatus = true
                            UTMSPStateStorage.currentStateIndex = 1
                            UTMSPStateStorage.currentNotificationIndex = 2
                        }
                        else{
                            submitFlightPlan.enabled = true
                        }
                    }
                }

                QGCButton {
                    id:             deleteFlightPlan
                    text:           qsTr("Remove Flight Plan")
                    visible:        UTMSPStateStorage.removeFlightPlanState
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    onClicked:{
                        removeFlightPlanTriggered()
                        deleteFlightPlan.visible = false
                        geoSwitch.enabled = true
                        QGroundControl.utmspManager.utmspVehicle.triggerActivationStatusBar(false)
                        QGroundControl.utmspManager.utmspVehicle.loadTelemetryFlag(false)
                        UTMSPStateStorage.startTimeStamp = ""
                        UTMSPStateStorage.showActivationTab = false
                        UTMSPStateStorage.flightID = ""
                        UTMSPStateStorage.enableMissionUploadButton = false
                        UTMSPStateStorage.indicatorIdleStatus = true
                        UTMSPStateStorage.indicatorApprovedStatus = false
                        UTMSPStateStorage.indicatorActivatedStatus = false
                        UTMSPStateStorage.indicatorDisplayStatus = false
                        UTMSPStateStorage.currentStateIndex = 0
                        UTMSPStateStorage.currentNotificationIndex = 6
                    }
                }
            }
        }
    }
}
