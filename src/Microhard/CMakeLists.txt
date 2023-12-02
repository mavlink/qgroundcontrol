set(SOURCES
        MicrohardHandler.cc
        MicrohardManager.cc
        MicrohardSettings.cc
        )

find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Network REQUIRED)

add_library(Microhard ${SOURCES})
target_link_libraries(Microhard PUBLIC FactSystem)
target_link_libraries(Microhard PUBLIC Settings)
target_link_libraries(Microhard PUBLIC Qt6::Core)
target_link_libraries(Microhard PUBLIC Qt6::Network)
target_include_directories(Microhard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
