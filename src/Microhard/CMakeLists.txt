set(SOURCES
        MicrohardHandler.cc
        MicrohardManager.cc
        MicrohardSettings.cc
        )

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Network REQUIRED)

add_library(Microhard ${SOURCES})
target_link_libraries(Microhard PUBLIC FactSystem)
target_link_libraries(Microhard PUBLIC Settings)
target_link_libraries(Microhard PUBLIC Qt5::Core)
target_link_libraries(Microhard PUBLIC Qt5::Network)
target_include_directories(Microhard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
