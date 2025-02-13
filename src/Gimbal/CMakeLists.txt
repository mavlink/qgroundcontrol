target_sources(QGC
    PRIVATE
        Gimbal.cc
        Gimbal.h
        GimbalController.cc
        GimbalController.h
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
