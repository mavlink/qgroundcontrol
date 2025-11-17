# Code coverage configuration for QGroundControl
# Enabled via: cmake -DQGC_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug

if(NOT QGC_ENABLE_COVERAGE)
    return()
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Code coverage requires Debug build, but CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
    return()
endif()

message(STATUS "Code coverage instrumentation enabled")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Using GCC coverage (gcov/lcov)")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE --coverage -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE --coverage)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Using Clang source-based coverage (llvm-cov)")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -fprofile-instr-generate -fcoverage-mapping -O0 -g)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -fprofile-instr-generate)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(WARNING "Code coverage not supported for MSVC. Use Visual Studio Enterprise or OpenCppCoverage.")

else()
    message(WARNING "Code coverage not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()
