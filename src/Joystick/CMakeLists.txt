
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

if(LINUX)
        find_package(SDL2 REQUIRED)
        if (IS_DIRECTORY ${SDL2_INCLUDE_DIRS})
            include_directories(${SDL2_INCLUDE_DIRS})
            string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
            target_link_libraries(Joystick PRIVATE ${SDL2_LIBRARIES})
        else()
            include_directories(${SDL2_DIR})
            target_link_libraries(Joystick PRIVATE SDL2::SDL2)
        endif()

elseif(APPLE)
        include_directories(${CMAKE_SOURCE_DIR}/libs/lib/Frameworks/SDL2.framework/Headers)
        target_link_libraries(Joystick PRIVATE -F${CMAKE_SOURCE_DIR}/libs/lib/Frameworks "-framework SDL2")
        set_target_properties(Joystick PROPERTIES
                        MACOSX_BUNDLE YES
        )
elseif(WIN32)
	include_directories(libs/lib/sdl2/msvc/include)
	# Support both 32 and 64 bit builds
	if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
		set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/libs/lib/sdl2/msvc/lib/x64/SDL2.lib;${CMAKE_SOURCE_DIR}/libs/lib/sdl2/msvc/lib/x64/SDL2main.lib")
	else ()
		set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/libs/lib/sdl2/msvc/lib/x86/SDL2.lib;${CMAKE_SOURCE_DIR}/libs/lib/sdl2/msvc/lib/x86/SDL2main.lib")
	endif ()

	string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
	
	target_link_libraries(Joystick ${SDL2_LIBRARIES})
endif()

