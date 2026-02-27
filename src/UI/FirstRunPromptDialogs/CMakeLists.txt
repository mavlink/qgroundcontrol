qt_add_library(FirstRunPromptDialogsModule STATIC)

qt_add_qml_module(FirstRunPromptDialogsModule
    URI QGroundControl.FirstRunPromptDialogs
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        OfflineVehicleFirstRunPrompt.qml
        UnitsFirstRunPrompt.qml
    NO_PLUGIN
)
