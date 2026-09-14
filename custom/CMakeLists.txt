# Synclair Vision custom-build composition and resources.

add_subdirectory("${CMAKE_SOURCE_DIR}/src/SynclairVision" "${CMAKE_BINARY_DIR}/SynclairVision")
target_link_libraries(SynclairVisionUIModule PRIVATE mavlink)
target_include_directories(SynclairVisionUIModule PRIVATE "${CMAKE_BINARY_DIR}")

set(SYNCLAIR_QML_OVERRIDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewCustomLayer.qml"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewCustomLayer.qml"
    PROPERTIES QT_RESOURCE_ALIAS "QGroundControl/FlyView/FlyViewCustomLayer.qml"
)
qt_add_resources(${CMAKE_PROJECT_NAME} synclair_qml_overrides
    PREFIX "/Custom/qml"
    FILES ${SYNCLAIR_QML_OVERRIDES}
)

set(CUSTOM_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CustomPlugin.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CustomPlugin.h"
    PARENT_SCOPE
)

set(CUSTOM_LIBRARIES
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,SynclairVisionUIModule>"
    PARENT_SCOPE
)

set(CUSTOM_INCLUDE_DIRECTORIES
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PARENT_SCOPE
)

set(CUSTOM_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="CustomPlugin.h"
    CUSTOMCLASS=CustomPlugin
    PARENT_SCOPE
)

list(APPEND QGC_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/custom.qrc"
)

set(CPACK_IFW_PACKAGE_LOGO2 "${CMAKE_CURRENT_SOURCE_DIR}/res/svlogo(2).svg")
