find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

qt_add_library(GeoTest
	STATIC
        GeoTest.cc
        GeoTest.h
)

target_link_libraries(GeoTest
    PRIVATE
        Geo
	PUBLIC
		Qt6::Positioning
		qgcunittest
)

target_include_directories(GeoTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
