
set(EXTRA_SRC)

if (ANDROID)
	list(APPEND EXTRA_SRC
		JoystickAndroid.cc
	)
endif()

add_library(Joystick
	Joystick.cc
	JoystickManager.cc
	JoystickSDL.cc
	JoystickMavCommand.cc
	${EXTRA_SRC}
)

target_link_libraries(Joystick
        PRIVATE
            ui
	PUBLIC
		qgc
		ui
)

target_include_directories(Joystick PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
	target_link_libraries(Joystick PUBLIC sdl2)
else()
	find_package(SDL2 REQUIRED)
	if (IS_DIRECTORY "${SDL2_INCLUDE_DIRS}")
		include_directories(${SDL2_INCLUDE_DIRS})
		string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
		target_link_libraries(Joystick PRIVATE ${SDL2_LIBRARIES})
	else()
		include_directories(${SDL2_DIR})
		target_link_libraries(Joystick PRIVATE SDL2::SDL2)
	endif()
endif()
