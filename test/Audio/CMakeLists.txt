
qt_add_library(AudioTest
	STATIC
		AudioOutputTest.cc AudioOutputTest.h
)

target_link_libraries(AudioTest
        PRIVATE
                Audio
	PUBLIC
		qgcunittest
)

target_include_directories(AudioTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
