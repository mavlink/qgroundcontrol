target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GeoTest.cc
        GeoTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
