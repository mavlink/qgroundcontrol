find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(Joystick STATIC
	Joystick.cc
	Joystick.h
	JoystickManager.cc
	JoystickManager.h
	JoystickMavCommand.cc
	JoystickMavCommand.h
)

if(ANDROID)
	target_sources(Joystick
		PRIVATE
			JoystickAndroid.cc
			JoystickAndroid.h
	)

	target_link_libraries(Joystick
		PUBLIC
			Qt6::CorePrivate
	)
else()
	target_sources(Joystick
		PRIVATE
			JoystickSDL.cc
			JoystickSDL.h
	)

	target_link_libraries(Joystick
		PUBLIC
			sdl2
	)
endif()

target_link_libraries(Joystick
	PUBLIC
		Qt6::Core
		qgc
		Vehicle
)

target_include_directories(Joystick PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
