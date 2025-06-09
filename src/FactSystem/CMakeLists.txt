target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Fact.cc
        Fact.h
        FactGroup.cc
        FactGroup.h
        FactMetaData.cc
        FactMetaData.h
        FactValueSliderListModel.cc
        FactValueSliderListModel.h
        ParameterManager.cc
        ParameterManager.h
        SettingsFact.cc
        SettingsFact.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_library(FactSystemModule STATIC)

qt_add_qml_module(FactSystemModule
    URI QGroundControl.FactSystem
    VERSION 1.0
    RESOURCE_PREFIX /qml
    NO_PLUGIN # Required by Qt 6.6.3. TODO: Remove when no longer supported.
)

add_subdirectory(FactControls)
