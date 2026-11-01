# Script mode: cmake -DAPP=<app binary> [-DNM=<nm>] [-DSYMBOL_PREFIX=_] -P SingleCoreCheck.cmake
# Fails when the Android or iOS app binary defines GStreamer core symbols, i.e. it carries a second core besides
# GStreamer::mobile (Mobile.cmake). On Apple (SYMBOL_PREFIX "_") nm lists the defined global symbols; elsewhere
# every defined symbol, so a copy with hidden visibility is caught too.
cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/Mobile.cmake")

if(NOT NM)
    set(NM nm)
endif()
if(SYMBOL_PREFIX)
    set(_nm_args -gU)
else()
    set(_nm_args --defined-only)
endif()
execute_process(
    COMMAND "${NM}" ${_nm_args} "${APP}"
    OUTPUT_VARIABLE _nm_output
    RESULT_VARIABLE _nm_result
)
if(NOT _nm_result EQUAL 0)
    message(FATAL_ERROR "GStreamer core check: '${NM} ${_nm_args} ${APP}' failed (${_nm_result})")
endif()

qgc_gst_app_core_symbols("${_nm_output}" "${SYMBOL_PREFIX}" _core_symbols)
if(_core_symbols)
    message(
        FATAL_ERROR
            "GStreamer core check: ${APP} defines GStreamer core symbols (${_core_symbols}), so it links a second "
            "core. Only GStreamer::mobile may link the GStreamer SDK's archives "
            "(cmake/GStreamer/platform/Android.cmake, platform/IOS.cmake); the app takes GStreamer from it."
    )
endif()
