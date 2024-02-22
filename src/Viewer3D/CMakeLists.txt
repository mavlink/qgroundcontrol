find_package(Qt6 REQUIRED COMPONENTS Core Quick3D)

qt_add_library(Viewer3D STATIC
    Viewer3DManager.cc
    CityMapGeometry.cc
    CityMapGeometry.h
    earcut.hpp
    OsmParser.cc
    OsmParser.h
    Viewer3DManager.h
    Viewer3DQmlBackend.cc
    Viewer3DQmlBackend.h
    Viewer3DQmlVariableTypes.h
    Viewer3DUtils.cc
    Viewer3DUtils.h
    Viewer3DTileReply.h
    Viewer3DTileReply.cc
    Viewer3DTerrainGeometry.h
    Viewer3DTerrainGeometry.cc
    Viewer3DTerrainTexture.h
    Viewer3DTerrainTexture.cc
    Viewer3DTileQuery.h
    Viewer3DTileQuery.cc

)

target_link_libraries(Viewer3D
	PUBLIC
		qgc
        Qt6::Quick3D
)

target_include_directories(Viewer3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
