add_subdirectory(preferences)

qt_add_library(uiTest STATIC)

target_link_libraries(QmlControlsTest
	PUBLIC
		qgc
		qgcunittest
		UiPreferencesQmlTest
)

target_include_directories(QmlControlsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
