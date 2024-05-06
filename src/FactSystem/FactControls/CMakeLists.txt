find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(FactControls STATIC
    FactPanelController.cc
	FactPanelController.h
)

target_link_libraries(FactControls
	PRIVATE
		Qt6::Qml
		AutoPilotPlugins
		FactSystem
		QGC
		Utilities
		Vehicle
	PUBLIC
		Qt6::Core
)

target_include_directories(FactControls INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_qml_module(FactControls
#     URI QGroundControl.FactControls
#     VERSION 1.0
#     QML_FILES
# 		AltitudeFactTextField.qml
# 		FactBitmask.qml
# 		FactCheckBox.qml
# 		FactCheckBoxSlider.qml
# 		FactComboBox.qml
# 		FactLabel.qml
# 		FactTextField.qml
# 		FactTextFieldGrid.qml
# 		FactTextFieldRow.qml
# 		FactTextFieldSlider.qml
# 		FactValueSlider.qml
#         LabelledFactComboBox.qml
#         LabelledFactSlider.qml
#         LabelledFactTextField.qml
#     OUTPUT_TARGETS FactControls_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
# 		QtQuick
# 		QtQuick.Controls
# 		QtQuick.Dialogs
# 		QtQuick.Layouts
# 		QGroundControl
# 		QGroundControl.Palette
# 		QGroundControl.Controls
# 		QGroundControl.FactSystem
# 		QGroundControl.ScreenTools
#     DEPENDENCIES
#         QtCore
# )