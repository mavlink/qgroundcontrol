# ============================================================================
# Fact System Module
# Parameter management and data binding infrastructure
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Fact.cc
        Fact.h
        FactGroup.cc
        FactGroup.h
        FactGroupListModel.cc
        FactGroupListModel.h
        FactGroupWithId.cc
        FactGroupWithId.h
        FactMetaData.cc
        FactMetaData.h
        FactValueSliderListModel.cc
        FactValueSliderListModel.h
        ParamHashCheckStateMachine.cc
        ParamHashCheckStateMachine.h
        ParamRequestListStateMachine.cc
        ParamRequestListStateMachine.h
        ParamRequestListPhase1StateMachine.cc
        ParamRequestListPhase1StateMachine.h
        ParamRequestListPhase2StateMachine.cc
        ParamRequestListPhase2StateMachine.h
        ParameterManager.cc
        ParameterManager.h
        SettingsFact.cc
        SettingsFact.h
        WaitForHashCheckParamValueState.cc
        WaitForHashCheckParamValueState.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Fact Controls UI
# ----------------------------------------------------------------------------
add_subdirectory(FactControls)
