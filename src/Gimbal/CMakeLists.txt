target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Gimbal.cc
        Gimbal.h
        GimbalController.cc
        GimbalController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
