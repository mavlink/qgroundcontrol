
qt_add_library(Audio
	AudioOutput.cc
)

target_link_libraries(Audio
	PRIVATE
		qgc
	PUBLIC
		Qt6::Core
		Qt6::Qml
		Qt6::TextToSpeech
		Qt6::Widgets
)

target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
