# The supervision of a connection that is kept up, as a value: when an attempt opens, how long a retry waits and when
# the connection gives up. It includes nothing else of src/GPS.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSConnectionSupervisor.cc
        GPSConnectionSupervisor.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
