
add_library(ViewWidgets
	ViewWidgetController.cc
)

target_link_libraries(ViewWidgets
	PUBLIC
		qgc
)

add_custom_target(ViewWidgetsQml 
SOURCE
	ViewWidget.qml
)

target_include_directories(ViewWidgets INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
