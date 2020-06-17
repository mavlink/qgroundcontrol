add_library(FactControls
    FactPanelController.cc
)

add_custom_target(FactControlsQml
    SOURCES
    AltitudeFactTextField.qml  
    FactCheckBox.qml  
    FactLabel.qml          
    FactTextField.qml     
    FactTextFieldSlider.qml
    FactBitmask.qml            
    FactComboBox.qml  
    FactTextFieldGrid.qml  
    FactTextFieldRow.qml  
    FactValueSlider.qml
)

target_link_libraries(FactControls qgc)