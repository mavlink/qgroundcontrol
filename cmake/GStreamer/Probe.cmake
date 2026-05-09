# Feature-test helpers for GStreamer headers/symbols — wraps
# check_cxx_source_compiles for callers that need to gate features on a header
# or symbol being present (HwBuffers, Orchestrator, Linux DMABuf probe).

include_guard(GLOBAL)
include(CheckCXXSourceCompiles)

# qgc_check_gst_header(VAR <out_var> HEADER <header_path> SYMBOL <symbol> [TARGET <gst_target>])
# Compile-tests that <header> can be included and <symbol> referenced when
# linked against <gst_target> (defaults to GStreamer::GStreamer, falling back
# to GStreamerMobile on Android where that's the primary target).
# Result is set in OUT_VAR (TRUE/FALSE in PARENT_SCOPE) and cached so the test
# isn't re-run on incremental reconfigures (check_cxx_source_compiles already
# caches by VAR name).
function(qgc_check_gst_header)
    cmake_parse_arguments(ARG "" "VAR;HEADER;SYMBOL;TARGET" "" ${ARGN})
    foreach(_req IN ITEMS VAR HEADER SYMBOL)
        if(NOT ARG_${_req})
            message(FATAL_ERROR "qgc_check_gst_header: ${_req} is required")
        endif()
    endforeach()

    if(NOT ARG_TARGET)
        if(ANDROID AND TARGET GStreamerMobile)
            set(ARG_TARGET GStreamerMobile)
        else()
            set(ARG_TARGET GStreamer::GStreamer)
        endif()
    endif()

    set(_saved "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES ${ARG_TARGET})
    check_cxx_source_compiles("
        #include <${ARG_HEADER}>
        int main() { (void)${ARG_SYMBOL}; return 0; }
    " ${ARG_VAR})
    set(CMAKE_REQUIRED_LIBRARIES "${_saved}")
    set(${ARG_VAR} "${${ARG_VAR}}" PARENT_SCOPE)
endfunction()
