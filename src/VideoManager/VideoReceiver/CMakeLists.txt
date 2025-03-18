find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(QGC PRIVATE VideoReceiver.h)

target_link_libraries(QGC PUBLIC Qt6::Core)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(GStreamer)
add_subdirectory(QtMultimedia)
