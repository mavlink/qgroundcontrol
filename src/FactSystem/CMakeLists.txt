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

add_subdirectory(FactControls)
