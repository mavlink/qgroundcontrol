target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/VehicleTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/VehicleTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/VehicleTestManualConnect.h
        ${CMAKE_CURRENT_SOURCE_DIR}/MissionTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/MissionTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ParameterTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ParameterTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/CommsTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/CommsTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TerrainTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/TerrainTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/JsonTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/StateMachineTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/StateMachineTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
