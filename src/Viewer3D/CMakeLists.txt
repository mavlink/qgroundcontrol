add_subdirectory(Viewer3DQml)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Network Positioning Quick3D Xml)

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
    OsmParserThread.cc
    OsmParserThread.h
)

target_link_libraries(Viewer3D
	PUBLIC
		qgc
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        Qt6::Network
        Qt6::Positioning
        Qt6::Quick3D
        Qt6::Xml
)

target_include_directories(Viewer3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
