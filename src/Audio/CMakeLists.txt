
set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		AudioOutputTest.cc
	)
endif()

add_library(Audio
	AudioOutput.cc
	${EXTRA_SRC}
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

if(BUILD_TESTING)
	add_qgc_test(AudioOutputTest)
endif()

