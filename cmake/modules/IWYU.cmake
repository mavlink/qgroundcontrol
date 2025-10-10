# ============================================================================
# IWYU.cmake
# Include-What-You-Use (IWYU) integration for header usage analysis
# Helps identify unnecessary includes and suggest proper forward declarations
# ============================================================================

# IWYU only works with Clang
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(INCLUDE_WHAT_YOU_USE_PROGRAM include-what-you-use)
    if(INCLUDE_WHAT_YOU_USE_PROGRAM)
        message(STATUS "QGC: Found include-what-you-use: ${INCLUDE_WHAT_YOU_USE_PROGRAM}")
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE_PROGRAM})
        set(CMAKE_C_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE_PROGRAM})
    endif()
endif()
