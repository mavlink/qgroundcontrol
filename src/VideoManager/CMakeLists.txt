target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        SubtitleWriter.cc
        SubtitleWriter.h
        VideoManager.cc
        VideoManager.h
)

add_subdirectory(VideoReceiver)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
