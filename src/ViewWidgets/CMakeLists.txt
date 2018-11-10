
add_library(ViewWidgets
	CustomCommandWidget.cc
	CustomCommandWidgetController.cc
	ViewWidgetController.cc
)

target_link_libraries(ViewWidgets
	PUBLIC
		qgc
)

target_include_directories(ViewWidgets INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
