find_package(Qt6 REQUIRED COMPONENTS Core)

add_subdirectory(libevents/libs/cpp)

qt_add_library(libevents_wrapper STATIC
        logging.cpp
)
target_include_directories(libevents_wrapper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libevents_wrapper PUBLIC Qt6::Core comm libevents)
