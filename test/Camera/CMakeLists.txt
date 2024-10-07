find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_library(CameraTest
    STATIC
        QGCCameraManagerTest.cc
        QGCCameraManagerTest.h
)

target_link_libraries(CameraTest
    PRIVATE
        Qt6::Test
        Camera
    PUBLIC
        qgcunittest
)

target_include_directories(CameraTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
