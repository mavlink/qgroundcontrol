set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Fact.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Fact.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FactGroup.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FactGroup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FactMetaData.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FactMetaData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FactValueSliderListModel.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FactValueSliderListModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ParameterManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ParameterManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SettingsFact.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SettingsFact.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

add_subdirectory(FactControls)
