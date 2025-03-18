find_package(Qt6 REQUIRED COMPONENTS Core QmlIntegration)

target_sources(QGC
    PRIVATE
        SubtitleWriter.cc
        SubtitleWriter.h
        VideoManager.cc
        VideoManager.h
)

add_subdirectory(VideoReceiver)

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
