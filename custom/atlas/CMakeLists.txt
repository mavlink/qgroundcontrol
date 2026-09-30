message(STATUS "ATLAS: personalizacao carregada")

set(ATLAS_BRAND_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/res/atlas_logo_white.svg"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/res/atlas_logo_white.svg"
    PROPERTIES QT_RESOURCE_ALIAS "atlas_logo_white.svg"
)

qt_add_resources(${CMAKE_PROJECT_NAME} atlas_brand
    PREFIX "/atlas"
    FILES ${ATLAS_BRAND_RESOURCES}
)
