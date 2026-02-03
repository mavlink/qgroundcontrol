# ============================================================================
# Viewer3D Module
# 3D visualization with terrain and city geometry
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Viewer3DManager.cc
        Viewer3DManager.h
        Viewer3DMapProvider.cc
        Viewer3DMapProvider.h
        Viewer3DGeoCoordinateType.h
        Viewer3DGeoCoordinateType.cc
        Viewer3DInstancing.cc
        Viewer3DInstancing.h
        Viewer3DTerrainGeometry.cc
        Viewer3DTerrainGeometry.h
        Viewer3DTerrainTexture.cc
        Viewer3DTerrainTexture.h
        Viewer3DTileQuery.cc
        Viewer3DTileQuery.h
        Viewer3DTileReply.cc
        Viewer3DTileReply.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Providers/Osm)

# ----------------------------------------------------------------------------
# 3D QML Module
# ----------------------------------------------------------------------------
qt_add_library(Viewer3DModule STATIC)

set_source_files_properties(Viewer3DQml/Images/city_3d_map_icon.svg PROPERTIES QT_RESOURCE_ALIAS City3DMapIcon.svg)
set_source_files_properties(Viewer3DQml/Models3D/Shaders/earthMaterial.frag PROPERTIES QT_RESOURCE_ALIAS ShaderFragment/earthMaterial.frag)
set_source_files_properties(Viewer3DQml/Models3D/Shaders/earthMaterial.vert PROPERTIES QT_RESOURCE_ALIAS ShaderVertex/earthMaterial.vert)
set_source_files_properties(Viewer3DQml/Viewer3DSettings.qml PROPERTIES QT_RESOURCE_ALIAS Viewer3DSettings.qml)
set_source_files_properties(Viewer3DQml/Models3D/Viewer3DModel.qml PROPERTIES QT_RESOURCE_ALIAS Models3D/Viewer3DModel.qml)

qt_add_qml_module(Viewer3DModule
    URI QGroundControl.Viewer3D
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        Viewer3DQml/Models3D/CameraLightModel.qml
        Viewer3DQml/Models3D/Viewer3DModel.qml
        Viewer3DQml/Drones/DronePart.qml
        Viewer3DQml/Drones/AnimatedPropeller.qml
        Viewer3DQml/Drones/DroneModelDjiF450.qml
        Viewer3DQml/Models3D/Viewer3DVehicleItems.qml
        Viewer3DQml/Viewer3DProgressBar.qml
        Viewer3DQml/Viewer3DSettings.qml
        Viewer3DQml/Viewer3DShowAction.qml
    RESOURCES
        Viewer3DQml/Images/city_3d_map_icon.svg
        Viewer3DQml/Models3D/Shaders/earthMaterial.frag
        Viewer3DQml/Models3D/Shaders/earthMaterial.vert
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
    IMPORTS
        QtPositioning
        QtQuick
        QtQuick3D
        QtQuick3D.Helpers
    NO_PLUGIN
)
