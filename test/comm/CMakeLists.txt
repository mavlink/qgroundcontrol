
qt_add_library(CommTest STATIC)

target_link_libraries(FactSystemTest
	PUBLIC
		qgc
		qgcunittest
)

target_include_directories(FactSystemTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_qml_module(CommTest
    URI commtest
    VERSION 1.0
    QML_FILES
    	MockLinkOptionsDlg.qml
	IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
)
