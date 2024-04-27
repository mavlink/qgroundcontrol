
qt_add_library(GeoTest
	STATIC
                GeoTest.cc
                GeoTest.h
)

target_link_libraries(GeoTest
        PRIVATE
                Geo
	PUBLIC
		qgc
		qgcunittest
)

target_include_directories(GeoTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
