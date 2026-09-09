if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.25)
    project(AndroidSerialWriteTests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_AUTOMOC ON)
    find_package(Qt6 REQUIRED COMPONENTS Core Test)
    enable_testing()
endif()

qt_add_executable(QGCAndroidSerialWriteTest AndroidSerialWriteTest.cc
                  ../../src/Android/qtandroidserialport/AndroidSerialWrite.cc
)
target_include_directories(QGCAndroidSerialWriteTest PRIVATE ../../src/Android/qtandroidserialport)
target_link_libraries(QGCAndroidSerialWriteTest PRIVATE Qt6::Core Qt6::Test)
add_test(NAME AndroidSerialWriteTest COMMAND QGCAndroidSerialWriteTest)
set_tests_properties(AndroidSerialWriteTest PROPERTIES LABELS "Unit;Comms" TIMEOUT 30)
