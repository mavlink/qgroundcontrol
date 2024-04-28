add_subdirectory(preferences)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(uiTest STATIC)

target_link_libraries(QmlControlsTest
	PUBLIC
		UiPreferencesQmlTest
)

target_include_directories(QmlControlsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
