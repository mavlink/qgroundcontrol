find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(MAVLinkTest
    STATIC
)

target_link_libraries(MAVLinkTest
    PRIVATE
        Qt6::Test
        MAVLink
    PUBLIC
        qgcunittest
)

target_include_directories(MAVLinkTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
