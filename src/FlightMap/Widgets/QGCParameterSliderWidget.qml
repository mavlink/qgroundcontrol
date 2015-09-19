/////////////////////////////////////////////////////////////////////////
////// QGCParameterSliderWidget.qml -- Trieste Devlin, Sept 2015   //////
////// QGroundControl widget -- sliding parameter tuner	           //////
////// Note: Functional EXCEPT: slider view doesn't update         //////
////// 	     when a parameter is changed from Setup tab            //////
/////////////////////////////////////////////////////////////////////////

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

FactPanel {
    id: panel
    property var qgcView: null // Temporary hack for broken QGC parameter validation implementation

    ParameterEditorController {
	    id: controller;
	    factPanel: panel

	    onShowErrorMessage: {
	        showMessage("Parameter Load Errors", errorMsg, StandardButton.Ok)
	    }
	}

	ScrollView {
		id:	scroll
		anchors.fill: parent

			Column {
				id: slidersColumn
		    	width:  scroll.viewport.width
		    	spacing: 5

		    	QGCButton { // click to sync sliders if params changed in Plan tab //TODO: NOT FUNCTIONAL
		    		id: syncButton
		    		width: parent.width
		    		height: 40
		    		text: "Sync sliders with Setup tab parameters"
		    		onClicked: {
		    			for(i = 1; i < paramRepeater.count + 1; i++){
			    			paramRepeater.itemAt(i).displayedValue = paramRepeater.itemAt(i).myFact.value
			    			paramRepeater.itemAt(i).state = "updated"
			    		}
		    		}
		    	}

		    	Rectangle { //slider legend display
		    		id: legend
		    		width: parent.width
		    		height: 40
		    		radius: ScreenTools.defaultFontPixelSize * (0.5)
            		color:  Qt.rgba(0,0,0,0.65);

            		Text {
            			color: "white"
            			y: legend.height - 15
            			x: 5
            			text: "PARAM_NAME"
            		}

		    		Text {
		    			color: "green"
		    			y: legend.height - 15
                		x: legend.width / 2 - 40
		    			text: "currently onboard"
		    		}
		    		Text {
		    			color: "white"
		    			text: "default"
		    			y: legend.height - 15
                		x: legend.width - 70
		    		}
		    		Text {
		    			color: "black"
		    			y: 5
                		x: legend.width - 70
		    			text: "maximum"
		    		}
		    	}

// TODO: add button to sync parameters from Plan tab (value = myFact.value)
// iterate through Repeater

			    Repeater {
			    	id: paramRepeater
			        model: ["MC_ROLL_P", "MC_ROLLRATE_P", "MC_PITCH_P", "MC_PITCHRATE_P", "MC_YAW_P", "MC_YAWRATE_P", "MC_ROLLRATE_D", "MC_PITCHRATE_D", "MC_YAWRATE_D", "MC_ROLLRATE_I", "MC_PITCHRATE_I", "MC_YAWRATE_I", "MC_ROLLRATE_MAX", "MC_PITCHRATE_MAX", "MC_YAWRATE_MAX"]

					Item { //QGCSlider
					    id: slider;
					    height: 50
					    width: parent.width
					    Layout.alignment: Qt.alignLeft

					    property Fact myFact: controller.getParameterFact(-1, modelData)

					    property real value: myFact.value.toFixed(5) // actual onboard value
					    property real displayedValue: myFact.value.toFixed(5) // slider bar value
					    property real def: myFact.defaultValue.toFixed(5)

					    property real minimum: myFact.min.toFixed(3)
					    property real maximum: myFact.defaultValue*2
					    property int length: width - handle.width
					    property string param: myFact.name

					    states: [
					        State {
					            name: "updated"
					            PropertyChanges {target: labelRect; color: Qt.rgba(1,1,1,0.75); value: displayedValue}
					        },
					        State {
					            name: "not_updated"
					            PropertyChanges {target: labelRect; color: Qt.rgba(1,0,0,0.75)}
					        }
					    ]

					        QGCButton {
					            id: confirmButton
					            text: "send value"

					            onClicked: {
					                slider.state = "updated";
					                slider.value = slider.displayedValue;
					            }
					        }

					        Rectangle { // slider background box
					            anchors.fill:   parent
					            radius:         ScreenTools.defaultFontPixelSize * (0.5)
					            color:          Qt.rgba(0,0,0,0.65);

					            QGCLabel { // max label
					                y: 5
					                x: slider.length - 15
					                text: slider.maximum.toFixed(3)
					            }

					            QGCLabel { // param name
					                y: parent.height - 15
					                x: 5
					                color: "white"
					                font.pointSize:12
					                text: slider.param
					            }

					            QGCLabel { // display default val
					                y: parent.height - 15
					                x: slider.length - 15
					                color: "white"
					                font.pointSize: 12
					                text: slider.def
					            }

					            QGCLabel { // display onboard value
					                y: parent.height - 15
					                x: slider.length / 2
					                color: "green"
					                font.pointSize: 12
					                text: slider.value.toFixed(5)
					            }
					        }

					        Rectangle { // slider bar
					            anchors.left:       parent.left
					            anchors.leftMargin: ScreenTools.defaultFontPixelSize * (0.15)
					            radius:             ScreenTools.defaultFontPixelSize * (0.15)
					            height:             ScreenTools.defaultFontPixelSize * (0.15)
					            width:              handle.x - x
					            color:              "#69bb17"
					            anchors.verticalCenter: parent.verticalCenter
					        }

					        Rectangle { // current (displayed) value bubble
					            id:                 labelRect
					            width:              label.width + 6
					            height:             label.height + 2
					            radius:             ScreenTools.defaultFontPixelSize * (0.33)
					            smooth:             true
					            color:              Qt.rgba(1,1,1,0.75);
					            border.width:       ScreenTools.defaultFontPixelSize * (0.083)
					            border.color:       Qt.rgba(0,0,0,0.45);
					            anchors.bottom:     handle.top
					            anchors.bottomMargin: ScreenTools.defaultFontPixelSize * (0.33)
					            x: Math.max(Math.min(handle.x + (handle.width - width ) / 2, slider.width - width), 0)
					            QGCLabel{
					                id:     label
					                color:  "black"
					                text:   slider.displayedValue.toFixed(5) //**
					                font.pointSize: 8
					                width:  font.pixelSize * 3.5
					                anchors.horizontalCenter:   labelRect.horizontalCenter
					                horizontalAlignment:        Text.AlignHCenter
					                anchors.verticalCenter:     labelRect.verticalCenter
					            }
					        }

					        Rectangle { // slider draggable handle
					            id:         handle;
					            smooth:     true
					            width:      ScreenTools.defaultFontPixelSize * (1.5);
					            y:          (slider.height - height) / 2;
					            x:          (slider.displayedValue - slider.minimum) * slider.length / (slider.maximum - slider.minimum) //**

					            height:     width
					            radius:     width / 2
					            gradient:   normalGradient
					            border.width: 2
					            border.color: "white"

					            Gradient {
					                id: normalGradient
					                GradientStop { position: 0.0;  color: "#b0b0b0" }
					                GradientStop { position: 0.66; color: "#909090" }
					                GradientStop { position: 1.0;  color: "#545454" }
					            }

					            MouseArea {
					                id: mouseRegion
					                hoverEnabled: false
					                anchors.fill: parent; drag.target: parent
					                drag.axis: Drag.XAxis; drag.minimumX: 0; drag.maximumX: slider.length
					                preventStealing: true
					                onPositionChanged: {
					                    slider.state = "not_updated"
					                    slider.displayedValue = (slider.maximum - slider.minimum) * handle.x / slider.length + slider.minimum; } //** 
					            }
					        }
						onValueChanged: myFact.value = value

					}//QGCSlider
				}//Repeater
			} //Column
	}//ScrollView
}//FactPanel
