target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AudioOutputTest.cc
        AudioOutputTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
