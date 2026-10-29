add_subdirectory(Registration)

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        UomDeviceAdapter.h
        Registration/VehicleUomRegistrationManager.cc
        Registration/VehicleUomRegistrationManager.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCUomRegistrationCore)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_UOM_REGISTRATION)
