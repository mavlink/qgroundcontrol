add_subdirectory(Viewer3DQml)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(Viewer3D STATIC)

find_package(Qt6 COMPONENTS Quick3D)
cmake_dependent_option(QGC_VIEWER3D "Enable Viewer3D" ON "Qt6Quick3D_FOUND" OFF)
if(QGC_VIEWER3D)
	message(STATUS "Viewer3D is Initialized")

        find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Positioning Qml Quick3D Xml)

	target_sources(Viewer3D
		PRIVATE
			CityMapGeometry.cc
			CityMapGeometry.h
			earcut.hpp
			OsmParser.cc
			OsmParser.h
			OsmParserThread.cc
			OsmParserThread.h
			Viewer3DManager.cc
			Viewer3DManager.h
			Viewer3DQmlBackend.cc
			Viewer3DQmlBackend.h
			Viewer3DQmlVariableTypes.h
			Viewer3DTerrainGeometry.cc
			Viewer3DTerrainGeometry.h
			Viewer3DTerrainTexture.cc
			Viewer3DTerrainTexture.h
			Viewer3DTileQuery.cc
			Viewer3DTileQuery.h
			Viewer3DTileReply.cc
			Viewer3DTileReply.h
			Viewer3DUtils.cc
			Viewer3DUtils.h
	)

	target_link_libraries(Viewer3D
                PRIVATE
                        Qt6::Network
                        QGC
                        QGCLocation
                        Settings
                        Vehicle
		PUBLIC
                        Qt6::Core
                        Qt6::Gui
                        Qt6::Positioning
                        Qt6::Quick3D
                        Qt6::Xml
	)

	target_include_directories(Viewer3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

	target_compile_definitions(Viewer3D PUBLIC QGC_VIEWER3D)
endif()
