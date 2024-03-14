find_package(Qt6 REQUIRED COMPONENTS Core TextToSpeech Widgets)

qt_add_library(Audio STATIC
	AudioOutput.cc
	AudioOutput.h
)

target_link_libraries(Audio
	PRIVATE
		Qt6::Widgets
		qgc
	PUBLIC
		Qt6::Core
		Qt6::TextToSpeech
)

target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
