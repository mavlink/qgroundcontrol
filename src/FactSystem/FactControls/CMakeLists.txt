target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FactPanelController.cc
        FactPanelController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_library(FactControlsModule STATIC)

qt_add_qml_module(FactControlsModule
    URI QGroundControl.FactControls
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        AltitudeFactTextField.qml
        FactBitmask.qml
        FactCheckBox.qml
        FactCheckBoxSlider.qml
        FactComboBox.qml
        FactLabel.qml
        FactTextField.qml
        FactTextFieldGrid.qml
        FactValueSlider.qml
        LabelledFactComboBox.qml
        LabelledFactTextField.qml
    NO_PLUGIN # Required by Qt 6.6.3. TODO: Remove when no longer supported.
)
