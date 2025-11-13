# ============================================================================
# Network Utilities
# Common networking helpers, URL utilities, and HTTP request configuration
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ApiKeyPool.cpp
        ApiKeyPool.h
        BandwidthThrottler.cpp
        BandwidthThrottler.h
        CircuitBreaker.cpp
        CircuitBreaker.h
        QGCNetworkHelper.cc
        QGCNetworkHelper.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link Qt6::HttpServer for HTTP status codes and methods
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::HttpServer)
