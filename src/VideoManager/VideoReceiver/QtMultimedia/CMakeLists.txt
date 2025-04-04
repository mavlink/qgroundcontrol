find_package(Qt6 REQUIRED COMPONENTS Core Multimedia QmlIntegration Quick)

# MultimediaQuickPrivate

qt_add_library(QtMultimediaReceiver STATIC
    QtMultimediaReceiver.cc
    QtMultimediaReceiver.h
)

target_link_libraries(QtMultimediaReceiver
    PRIVATE
        # Qt6::MultimediaPrivate
        Qt6::Quick
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Multimedia
        Qt6::QmlIntegration
        VideoReceiver
)

target_include_directories(QtMultimediaReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(QGC_DISABLE_UVC "Disable UVC Devices" OFF)
if(QGC_DISABLE_UVC)
    target_compile_definitions(QtMultimediaReceiver PUBLIC QGC_DISABLE_UVC)
else()
    target_sources(QtMultimediaReceiver
        PRIVATE
            UVCReceiver.cc
            UVCReceiver.h
    )
endif()
