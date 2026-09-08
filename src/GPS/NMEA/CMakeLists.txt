target_sources(${CMAKE_PROJECT_NAME} PRIVATE NmeaSourceManager.cc NmeaSourceManager.h NMEAUtils.cc NMEAUtils.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
