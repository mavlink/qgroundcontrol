
qt_add_library(FactSystemTest
	STATIC
		FactSystemTestBase.cc FactSystemTestBase.h
		FactSystemTestGeneric.cc FactSystemTestGeneric.h
		FactSystemTestPX4.cc FactSystemTestPX4.h
		ParameterManagerTest.cc ParameterManagerTest.h
)

target_link_libraries(FactSystemTest
	PUBLIC
		qgc
		qgcunittest
)

target_include_directories(FactSystemTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_qml_module(FactSystemTest
    URI factsystemtest
    VERSION 1.0
    QML_FILES
    	FactSystemTest.qml
	IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
)
