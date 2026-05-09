# Project-level resilient HTTP download with retry, checksum verify, and cache.
# GStreamer callers wrap this via gstreamer_resilient_download
# (cmake/GStreamer/Download.cmake).

include_guard(GLOBAL)

# qgc_parse_expected_hash(<value> <out_algo> <out_hex>)
# Splits "ALGO=hex" into the algorithm + hex digits expected by file(<ALGO>).
# Aborts with FATAL_ERROR if the value is malformed or the algorithm is not
# one of CMake's supported hash algorithms.
function(qgc_parse_expected_hash _HASH_VALUE _OUT_ALGO _OUT_EXPECTED)
    set(_valid MD5 SHA1 SHA224 SHA256 SHA384 SHA512 SHA3_224 SHA3_256 SHA3_384 SHA3_512)
    string(REGEX MATCH "^([A-Za-z0-9_]+)=(.+)$" _match "${_HASH_VALUE}")
    if(NOT _match)
        message(FATAL_ERROR "qgc_parse_expected_hash: Invalid EXPECTED_HASH format '${_HASH_VALUE}' (expected ALGO=hex)")
    endif()
    set(${_OUT_ALGO} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(${_OUT_EXPECTED} "${CMAKE_MATCH_2}" PARENT_SCOPE)
    list(FIND _valid "${CMAKE_MATCH_1}" _idx)
    if(_idx EQUAL -1)
        message(FATAL_ERROR "qgc_parse_expected_hash: Unsupported hash algorithm '${CMAKE_MATCH_1}'")
    endif()
endfunction()

# qgc_resilient_download(
#     FILENAME            <name>
#     DESTINATION_DIR     <dir>
#     RESULT_VAR          <var>           # set in PARENT_SCOPE; "" on ALLOW_FAILURE miss
#     URLS                <url>...        # tried in order; first success wins
#     [TIMEOUT             <secs>]        # default 120, override via QGC_DOWNLOAD_TIMEOUT
#     [INACTIVITY_TIMEOUT  <secs>]        # default  60, override via QGC_DOWNLOAD_INACTIVITY_TIMEOUT
#     [EXPECTED_HASH       <ALGO=hex>]    # cached + downloaded files validated
#     [ALLOW_FAILURE]                     # don't FATAL_ERROR if all URLs fail
#     [LOG_TAG             <prefix>]      # message() prefix, default "Download"
#     [FAILURE_HINT        <text>]        # appended to FATAL_ERROR on total failure
# )
function(qgc_resilient_download)
    cmake_parse_arguments(ARG "ALLOW_FAILURE"
        "FILENAME;DESTINATION_DIR;RESULT_VAR;TIMEOUT;INACTIVITY_TIMEOUT;EXPECTED_HASH;LOG_TAG;FAILURE_HINT"
        "URLS" ${ARGN})

    foreach(_required FILENAME DESTINATION_DIR RESULT_VAR)
        if(NOT ARG_${_required})
            message(FATAL_ERROR "qgc_resilient_download: ${_required} is required")
        endif()
    endforeach()
    if(NOT ARG_URLS)
        message(FATAL_ERROR "qgc_resilient_download: at least one URL is required")
    endif()

    if(NOT ARG_LOG_TAG)
        set(ARG_LOG_TAG "Download")
    endif()
    # Per-call > project cache var > built-in default. Cache vars exist so CI
    # can bump timeouts on slow runners without editing CMake source.
    if(NOT ARG_TIMEOUT)
        if(DEFINED QGC_DOWNLOAD_TIMEOUT)
            set(ARG_TIMEOUT ${QGC_DOWNLOAD_TIMEOUT})
        else()
            set(ARG_TIMEOUT 120)
        endif()
    endif()
    if(NOT ARG_INACTIVITY_TIMEOUT)
        if(DEFINED QGC_DOWNLOAD_INACTIVITY_TIMEOUT)
            set(ARG_INACTIVITY_TIMEOUT ${QGC_DOWNLOAD_INACTIVITY_TIMEOUT})
        else()
            set(ARG_INACTIVITY_TIMEOUT 60)
        endif()
    endif()

    set(_dest "${ARG_DESTINATION_DIR}/${ARG_FILENAME}")

    if(EXISTS "${_dest}")
        if(ARG_EXPECTED_HASH)
            qgc_parse_expected_hash("${ARG_EXPECTED_HASH}" _hash_algo _expected)
            file(${_hash_algo} "${_dest}" _cached_hash)
            if(NOT _cached_hash STREQUAL "${_expected}")
                message(STATUS "${ARG_LOG_TAG}: Cached ${ARG_FILENAME} failed ${_hash_algo} check, re-downloading")
                file(REMOVE "${_dest}")
            else()
                set(${ARG_RESULT_VAR} "${_dest}" PARENT_SCOPE)
                return()
            endif()
        else()
            message(STATUS "${ARG_LOG_TAG}: Using cached ${ARG_FILENAME} (no checksum verification)")
            set(${ARG_RESULT_VAR} "${_dest}" PARENT_SCOPE)
            return()
        endif()
    endif()

    file(MAKE_DIRECTORY "${ARG_DESTINATION_DIR}")
    set(_tmp "${_dest}.tmp")
    set(_tried_urls "")

    foreach(_url IN LISTS ARG_URLS)
        if(NOT _url)
            continue()
        endif()
        message(STATUS "${ARG_LOG_TAG}: Downloading ${ARG_FILENAME} from ${_url}")
        file(DOWNLOAD "${_url}" "${_tmp}"
            STATUS _status
            SHOW_PROGRESS
            TIMEOUT ${ARG_TIMEOUT}
            INACTIVITY_TIMEOUT ${ARG_INACTIVITY_TIMEOUT}
            TLS_VERIFY ON
        )
        list(GET _status 0 _code)
        if(_code EQUAL 0)
            if(ARG_EXPECTED_HASH)
                qgc_parse_expected_hash("${ARG_EXPECTED_HASH}" _hash_algo _expected)
                file(${_hash_algo} "${_tmp}" _actual_hash)
                if(NOT _actual_hash STREQUAL "${_expected}")
                    message(WARNING "${ARG_LOG_TAG}: ${_hash_algo} mismatch for ${ARG_FILENAME} from ${_url}")
                    file(REMOVE "${_tmp}")
                    list(APPEND _tried_urls "${_url}")
                    continue()
                endif()
            endif()
            file(RENAME "${_tmp}" "${_dest}")
            set(${ARG_RESULT_VAR} "${_dest}" PARENT_SCOPE)
            return()
        endif()
        list(GET _status 1 _error)
        message(WARNING "${ARG_LOG_TAG}: Failed to download from ${_url}: ${_error}")
        file(REMOVE "${_tmp}")
        list(APPEND _tried_urls "${_url}")
    endforeach()

    if(ARG_ALLOW_FAILURE)
        message(STATUS "${ARG_LOG_TAG}: All download URLs failed for ${ARG_FILENAME} (allowing failure). Tried: ${_tried_urls}")
        set(${ARG_RESULT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    set(_msg "${ARG_LOG_TAG}: All download URLs failed for ${ARG_FILENAME}.\nTried: ${_tried_urls}")
    if(ARG_FAILURE_HINT)
        string(APPEND _msg "\n${ARG_FAILURE_HINT}")
    endif()
    message(FATAL_ERROR "${_msg}")
endfunction()
