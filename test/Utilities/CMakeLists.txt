find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FileSystem/QGCFileDownloadTest.cc
        FileSystem/QGCFileDownloadTest.h
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Test)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} FileSystem)

qt_add_resources(${CMAKE_PROJECT_NAME} "UtilitiesTest_res"
    PREFIX "/"
    FILES
        arducopter.apj
)

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
