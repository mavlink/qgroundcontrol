# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.
#
# Original file location was Tests/RunCMake/CMakeLists.txt

set(RunCMakeDir ${CMAKE_CURRENT_LIST_DIR})

macro(add_RunCMake_test test)
  set(TEST_ARGS ${ARGN})
  if ("${ARGV1}" STREQUAL "TEST_DIR")
    if ("${ARGV2}" STREQUAL "")
      message(FATAL_ERROR "Invalid args")
    endif()
    set(Test_Dir ${ARGV2})
    list(REMOVE_AT TEST_ARGS 0)
    list(REMOVE_AT TEST_ARGS 0)
  else()
    set(Test_Dir ${test})
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "LCC")
    list(APPEND TEST_ARGS -DRunCMake_TEST_LCC=1)
  endif()
  if(NOT QT_RUN_CMAKE_SCRIPT_PATH)
    set(QT_RUN_CMAKE_SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${Test_Dir}/RunCMakeTest.cmake")
  endif()
  # Qt specific options
  list(APPEND TEST_ARGS
    -D_Qt6CTestMacros=${_Qt6CTestMacros}
  )
  add_test(NAME RunCMake.${test} COMMAND ${CMAKE_COMMAND}
    -DCMAKE_MODULE_PATH=${RunCMakeDir}
    -DRunCMake_GENERATOR_IS_MULTI_CONFIG=${_isMultiConfig}
    -DRunCMake_GENERATOR=${CMAKE_GENERATOR}
    -DRunCMake_GENERATOR_INSTANCE=${CMAKE_GENERATOR_INSTANCE}
    -DRunCMake_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
    -DRunCMake_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}
    -DRunCMake_MAKE_PROGRAM=${CMake_TEST_EXPLICIT_MAKE_PROGRAM}
    -DRunCMake_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/${Test_Dir}
    -DRunCMake_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/${test}
    ${${test}_ARGS}
    ${TEST_ARGS}
    -P "${QT_RUN_CMAKE_SCRIPT_PATH}"
    )
  unset(QT_RUN_CMAKE_SCRIPT_PATH)
endmacro()
