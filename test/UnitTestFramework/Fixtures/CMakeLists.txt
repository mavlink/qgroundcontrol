target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/CoordFixtures.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/CoordFixtures.h
        ${CMAKE_CURRENT_SOURCE_DIR}/MissionFixtures.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/MissionFixtures.h
        ${CMAKE_CURRENT_SOURCE_DIR}/RAIIFixtures.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/RAIIFixtures.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
