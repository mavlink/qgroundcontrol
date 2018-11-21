
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
		Qt5::Core
		Qt5::Qml
		Qt5::TextToSpeech
		Qt5::Widgets
)

target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_TESTING)
	add_qgc_test(AudioOutputTest)
endif()

