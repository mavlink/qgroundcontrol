# ============================================================================
# Sanitizer Configuration
# ============================================================================
# Provides AddressSanitizer, ThreadSanitizer, and UndefinedBehaviorSanitizer
# support for debugging memory errors and race conditions.
#
# Usage:
#   cmake -DSANITIZER=address ..
#   cmake -DSANITIZER=thread ..
#   cmake -DSANITIZER=undefined ..

if(NOT QGC_BUILD_TESTING)
    return()
endif()

set(SANITIZER "" CACHE STRING "Enable sanitizer (address, thread, undefined, memory)")

if(NOT SANITIZER)
    return()
endif()

# Sanitizers only work with GCC/Clang
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(WARNING "Sanitizers are only supported with GCC and Clang")
    return()
endif()

string(TOLOWER "${SANITIZER}" SANITIZER_LOWER)

if(SANITIZER_LOWER STREQUAL "address")
    message(STATUS "Sanitizer: AddressSanitizer (ASan) enabled")
    set(SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")

elseif(SANITIZER_LOWER STREQUAL "thread")
    message(STATUS "Sanitizer: ThreadSanitizer (TSan) enabled")
    set(SANITIZER_FLAGS "-fsanitize=thread")

elseif(SANITIZER_LOWER STREQUAL "undefined")
    message(STATUS "Sanitizer: UndefinedBehaviorSanitizer (UBSan) enabled")
    set(SANITIZER_FLAGS "-fsanitize=undefined -fno-omit-frame-pointer")

elseif(SANITIZER_LOWER STREQUAL "memory")
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(WARNING "MemorySanitizer is only supported with Clang")
        return()
    endif()
    message(STATUS "Sanitizer: MemorySanitizer (MSan) enabled")
    set(SANITIZER_FLAGS "-fsanitize=memory -fno-omit-frame-pointer")

else()
    message(WARNING "Unknown sanitizer: ${SANITIZER}")
    message(STATUS "Available sanitizers: address, thread, undefined, memory")
    return()
endif()

# Apply sanitizer flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}")
