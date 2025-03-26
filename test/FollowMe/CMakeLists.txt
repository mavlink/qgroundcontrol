find_package(Qt6 REQUIRED COMPONENTS Core Test)

target_sources(qgctest
    PRIVATE
        FollowMeTest.cc
        FollowMeTest.h
)

target_link_libraries(qgctest PRIVATE Qt6::Test)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
