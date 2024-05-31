find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(VideoReceiver STATIC VideoReceiver.h)

target_link_libraries(VideoReceiver
    PUBLIC
        Qt6::Core
    INTERFACE
        GStreamerReceiver
        QtMultimediaReceiver
)

add_subdirectory(GStreamer)
add_subdirectory(QtMultimedia)

# target_sources(GStreamerReceiver PUBLIC VideoReceiver.h)
# target_sources(QtMultimediaReceiver PUBLIC VideoReceiver.h)
target_include_directories(GStreamerReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(QtMultimediaReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
