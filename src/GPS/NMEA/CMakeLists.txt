target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEASourceManager.cc NMEASourceManager.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEAConnectionConfig.cc NMEAConnectionConfig.h)
