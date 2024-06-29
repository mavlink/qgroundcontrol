find_package(Qt6 REQUIRED COMPONENTS Core Qml)

qt_add_library(CommsTest STATIC)

qt_add_qml_module(CommsTest
    URI commstest
    VERSION 1.0
    QML_FILES
        MockLinkOptionsDlg.qml
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
)
