find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FactControls STATIC
    FactPanelController.cc
	FactPanelController.h
)

add_custom_target(FactControlsQml
	SOURCES
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

target_link_libraries(FactControls
	PRIVATE
		AutoPilotPlugins
		FactSystem
		qgc
		Utilities
	PUBLIC
		Vehicle
)

target_include_directories(FactControls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
