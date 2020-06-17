
add_library(Geo
	Constants.hpp
	Math.cpp
	Math.hpp
	MGRS.cpp
	MGRS.hpp
	PolarStereographic.cpp
	PolarStereographic.hpp
	QGCGeo.cc
	QGCGeo.h
	TransverseMercator.cpp
	TransverseMercator.hpp
	Utility.cpp
	Utility.h
	Utility.hpp
	UTMUPS.cpp
	UTMUPS.hpp
)

target_link_libraries(Geo
	PUBLIC
		qgc
)

target_include_directories(Geo
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

