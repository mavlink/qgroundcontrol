find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(VideoReceiver STATIC VideoReceiver.h)

target_link_libraries(VideoReceiver PUBLIC Qt6::Core)

target_include_directories(VideoReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(GStreamer)
add_subdirectory(QtMultimedia)
