add_subdirectory(Compression)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(UtilitiesTest STATIC
)

target_link_libraries(UtilitiesTest
    PRIVATE
        Qt6::Test
        Utilities
    PUBLIC
        qgcunittest
)

target_include_directories(UtilitiesTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
