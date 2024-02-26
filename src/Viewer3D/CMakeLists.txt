
qt_add_library(Viewer3D STATIC
    CityMapGeometry.cc
    CityMapGeometry.h
    earcut.hpp
    OsmParser.cc
    OsmParser.h
	Viewer3DManager.cc
    Viewer3DManager.h
    Viewer3DQmlBackend.cc
    Viewer3DQmlBackend.h
    Viewer3DQmlVariableTypes.h
    Viewer3DUtils.cc
    Viewer3DUtils.h
)

find_package(Qt6 REQUIRED COMPONENTS Quick3D)

target_link_libraries(Viewer3D
	PUBLIC
		qgc
        Qt6::Quick3D
)

target_include_directories(Viewer3D
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	)

