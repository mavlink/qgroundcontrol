add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(FileSystem)
add_subdirectory(Geo)

qt_add_resources(${CMAKE_PROJECT_NAME} "UtilitiesTest_res"
    PREFIX "/unittest"
    FILES
        arducopter.apj
)
