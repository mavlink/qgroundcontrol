# Host regression tests for the bundled Android backend. Only JNI is replaced.
set(_android_serial "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/Android")
find_package(Qt6 REQUIRED COMPONENTS CorePrivate)
add_executable(
    AndroidGPSCompatibilityTest
    AndroidGPSCompatibilityTest.cc
    ${_android_serial}/qtandroidserialport/qserialport.cpp
    ${_android_serial}/qtandroidserialport/qserialport_android.cpp
    ${_android_serial}/qtandroidserialport/qserialport_posix.cpp
    ${_android_serial}/qtandroidserialport/qserialportinfo.cpp
    ${_android_serial}/qtandroidserialport/qserialportinfo_android.cpp
    ${_android_serial}/../GPS/Transport/SerialGPSTransport.cc
)
target_compile_features(AndroidGPSCompatibilityTest PRIVATE cxx_std_20)
target_compile_options(AndroidGPSCompatibilityTest PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/AndroidHostPlatform.h)
target_include_directories(AndroidGPSCompatibilityTest
                           PRIVATE ${_android_serial} ${_android_serial}/qtandroidserialport
                                   ${_android_serial}/../GPS/Transport
)
target_link_libraries(AndroidGPSCompatibilityTest PRIVATE Qt6::Core Qt6::CorePrivate Qt6::Network Qt6::Test
                                                          QGCGPSTransport QGCLoggingCategory
)
add_test(NAME AndroidGPSCompatibilityTest COMMAND AndroidGPSCompatibilityTest)
set_tests_properties(AndroidGPSCompatibilityTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 30)
