# Script mode: cmake -DAPP=<app executable> -DNM=<nm> -P IOSSingleCoreCheck.cmake
# Fails when the iOS app binary defines GStreamer core symbols, i.e. it carries a second core besides GStreamer::mobile.
cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/IOS.cmake")

execute_process(
    COMMAND "${NM}" -gU "${APP}"
    OUTPUT_VARIABLE _nm_output
    RESULT_VARIABLE _nm_result
)
if(NOT _nm_result EQUAL 0)
    message(FATAL_ERROR "GStreamer iOS core check: '${NM} -gU ${APP}' failed (${_nm_result})")
endif()

_qgc_gst_ios_app_core_symbols("${_nm_output}" _core_symbols)
if(_core_symbols)
    message(
        FATAL_ERROR
            "GStreamer iOS core check: ${APP} defines GStreamer core symbols (${_core_symbols}), so it links a second "
            "core from the merged archive. Only GStreamer::mobile may link GStreamer_static "
            "(cmake/GStreamer/platform/IOS.cmake); the app takes GStreamer from it."
    )
endif()
