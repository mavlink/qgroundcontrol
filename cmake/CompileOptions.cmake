
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	include(CTest)
	enable_testing()
	add_definitions(-DUNITTEST_BUILD)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# clang and AppleClang
	add_compile_options(
		-Wall
		-Wextra
		-Wno-address-of-packed-member # ignore for mavlink
	)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# GCC
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
		add_compile_options(-fdiagnostics-color=always)
	endif()

	add_compile_options(
		-Wall
		-Wextra
	)
elseif (WIN32)
	add_definitions(-D_USE_MATH_DEFINES)
	add_compile_options(
		/wd4244 # warning C4244: '=': conversion from 'double' to 'float', possible loss of data
    )
endif()
