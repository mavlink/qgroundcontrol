find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(VideoManager PRIVATE VideoReceiver.h)

target_link_libraries(VideoManager PUBLIC Qt6::Core)

target_include_directories(VideoManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(GStreamer)
add_subdirectory(QtMultimedia)
