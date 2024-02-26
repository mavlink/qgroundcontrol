qt_add_library(FactControls STATIC
    FactPanelController.cc
)

add_custom_target(FactControlsQml
	SOURCES
		FactPanelController.h
		AltitudeFactTextField.qml
		FactBitmask.qml
		FactCheckBox.qml
		FactComboBox.qml
		FactLabel.qml
		FactTextFieldGrid.qml
		FactTextField.qml
		FactTextFieldRow.qml
		FactTextFieldSlider.qml
		FactValueSlider.qml
        LabelledFactComboBox.qml
        LabelledFactTextField.qml
        LabelledFactSlider.qml
)

target_link_libraries(FactControls qgc)
