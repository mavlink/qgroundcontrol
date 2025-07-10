if(NOT QGC_VIEWER3D)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
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
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Quick3D)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_VIEWER3D)

qt_add_library(Viewer3DModule STATIC)

set_source_files_properties(Images/city_3d_map_icon.svg PROPERTIES QT_RESOURCE_ALIAS City3DMapIcon.svg)
set_source_files_properties(Shaders/earthMaterial.frag PROPERTIES QT_RESOURCE_ALIAS ShaderFragment/earthMaterial.frag)
set_source_files_properties(Shaders/earthMaterial.vert PROPERTIES QT_RESOURCE_ALIAS ShaderVertex/earthMaterial.vert)

qt_add_qml_module(Viewer3DModule
    URI QGroundControl.Viewer3D
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        Viewer3DQml/Viewer3D.qml
        Viewer3DQml/Models3D/CameraLightModel.qml
        Viewer3DQml/Models3D/Viewer3DModel.qml
        Viewer3DQml/Drones/DroneModelDjiF450.qml
        Viewer3DQml/Drones/Djif450/DroneModel_arm_1/DroneModel_arm_1.qml
        Viewer3DQml/Drones/Djif450/DroneModel_arm_2/DroneModel_arm_2.qml
        Viewer3DQml/Drones/Djif450/DroneModel_arm_3/DroneModel_arm_3.qml
        Viewer3DQml/Drones/Djif450/DroneModel_arm_4/DroneModel_arm_4.qml
        Viewer3DQml/Drones/Djif450/DroneModel_BLDC_1/DroneModel_BLDC_1.qml
        Viewer3DQml/Drones/Djif450/DroneModel_BLDC_2/DroneModel_BLDC_2.qml
        Viewer3DQml/Drones/Djif450/DroneModel_BLDC_3/DroneModel_BLDC_3.qml
        Viewer3DQml/Drones/Djif450/DroneModel_BLDC_4/DroneModel_BLDC_4.qml
        Viewer3DQml/Drones/Djif450/DroneModel_propeller2_2/DroneModel_propeller2_2.qml
        Viewer3DQml/Drones/Djif450/DroneModel_propeller2_7/DroneModel_propeller2_7.qml
        Viewer3DQml/Drones/Djif450/DroneModel_propeller22_1/DroneModel_propeller22_1.qml
        Viewer3DQml/Drones/Djif450/DroneModel_propeller22_2/DroneModel_propeller22_2.qml
        Viewer3DQml/Drones/Djif450/DroneModel_Base_bottom_1/DroneModel_Base_bottom_1.qml
        Viewer3DQml/Drones/Djif450/DroneModel_Base_Top_1/DroneModel_Base_Top_1.qml
        Viewer3DQml/Models3D/Waypoint3DModel.qml
        Viewer3DQml/Models3D/Line3D.qml
        Viewer3DQml/Models3D/Viewer3DVehicleItems.qml
        Viewer3DQml/Viewer3DProgressBar.qml
    RESOURCES
        Images/city_3d_map_icon.svg
        Shaders/earthMaterial.frag
        Shaders/earthMaterial.vert
        Viewer3DQml/Drones/Djif450/DroneModel_arm_1/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_arm_2/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_arm_3/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_arm_4/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_BLDC_1/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_BLDC_2/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_BLDC_3/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_BLDC_4/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_propeller2_2/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_propeller2_7/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_propeller22_1/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_propeller22_2/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_Base_bottom_1/node.mesh
        Viewer3DQml/Drones/Djif450/DroneModel_Base_Top_1/node.mesh
    NO_PLUGIN
)
