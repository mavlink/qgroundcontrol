# cmake-lint: disable=C0103
cmake_minimum_required(VERSION 3.25)

project(QGCCheckResultCache LANGUAGES C)

if(NOT QGC_MODULE_DIR)
    message(FATAL_ERROR "QGC_MODULE_DIR is required")
endif()

include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include("${QGC_MODULE_DIR}/modules/CheckResultCache.cmake")

# A multi-architecture size mismatch caches 0 and keeps its per-architecture map in the build tree.
set(FIXTURE_SIZE_MAP
    0
    CACHE INTERNAL "CHECK_TYPE_SIZE: sizeof(long double)"
)
set(HAVE_FIXTURE_SIZE_MAP
    TRUE
    CACHE INTERNAL "Result of TRY_COMPILE"
)
set(FIXTURE_UNRELATED
    "state"
    CACHE INTERNAL "Unrelated internal state"
)

qgc_restore_check_results()

set(_seeded "")
foreach(
    _var IN ITEMS FIXTURE_HAVE_STDIO_H FIXTURE_MISSING_HEADER FIXTURE_HAVE_PRINTF FIXTURE_SIZEOF_INT
                  HAVE_FIXTURE_SIZEOF_INT FIXTURE_COMPILES
)
    if(DEFINED CACHE{${_var}})
        list(APPEND _seeded "${_var}")
    endif()
endforeach()
file(WRITE "${CMAKE_BINARY_DIR}/seeded.txt" "${_seeded}")

check_include_file(stdio.h FIXTURE_HAVE_STDIO_H)
check_include_file(qgc_check_result_cache_missing.h FIXTURE_MISSING_HEADER)
check_symbol_exists(printf stdio.h FIXTURE_HAVE_PRINTF)
check_type_size(int FIXTURE_SIZEOF_INT)
check_c_source_compiles("int main(void) { return 0; }" FIXTURE_COMPILES)

file(WRITE "${CMAKE_BINARY_DIR}/results.txt"
     "${FIXTURE_HAVE_STDIO_H}|${FIXTURE_MISSING_HEADER}|${FIXTURE_HAVE_PRINTF}|${FIXTURE_SIZEOF_INT}|"
     "${HAVE_FIXTURE_SIZEOF_INT}|${FIXTURE_COMPILES}|${FIXTURE_SIZEOF_INT_CODE}"
)
