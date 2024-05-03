find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_library(AudioTest
	STATIC
		AudioOutputTest.cc
		AudioOutputTest.h
)

target_link_libraries(AudioTest
        PRIVATE
        	Qt6::Test
                Audio
	PUBLIC
		qgcunittest
)

target_include_directories(AudioTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
