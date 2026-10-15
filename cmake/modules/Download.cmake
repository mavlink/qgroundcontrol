include_guard(GLOBAL)

# qgc_parse_expected_hash(<hash_value> <out_algo> <out_expected>)
# Splits an "ALGO=hex" EXPECTED_HASH into the file(<ALGO>) algorithm and its lowercase digest.
function(qgc_parse_expected_hash _hash_value _out_algo _out_expected)
    if(NOT ARGC EQUAL 3
       OR NOT _out_algo MATCHES "^[A-Za-z_][A-Za-z0-9_]*$"
       OR NOT _out_expected MATCHES "^[A-Za-z_][A-Za-z0-9_]*$"
    )
        message(FATAL_ERROR "qgc_parse_expected_hash: expected HASH_VALUE OUT_ALGORITHM OUT_DIGEST")
    endif()
    set(_valid
        MD5
        SHA1
        SHA224
        SHA256
        SHA384
        SHA512
        SHA3_224
        SHA3_256
        SHA3_384
        SHA3_512
    )
    string(REGEX MATCH "^([A-Za-z0-9_]+)=(.+)$" _match "${_hash_value}")
    if(NOT _match)
        message(
            FATAL_ERROR "qgc_parse_expected_hash: Invalid EXPECTED_HASH format '${_hash_value}' (expected ALGO=hex)"
        )
    endif()
    set(${_out_algo}
        "${CMAKE_MATCH_1}"
        PARENT_SCOPE
    )
    # file(<algorithm>) reports lowercase hex; accept digests written in either case.
    string(TOLOWER "${CMAKE_MATCH_2}" _expected_digest)
    set(${_out_expected}
        "${_expected_digest}"
        PARENT_SCOPE
    )
    list(FIND _valid "${CMAKE_MATCH_1}" _idx)
    if(_idx EQUAL -1)
        message(FATAL_ERROR "qgc_parse_expected_hash: Unsupported hash algorithm '${CMAKE_MATCH_1}'")
    endif()
endfunction()

# qgc_resilient_download(FILENAME <name> DESTINATION_DIR <dir> RESULT_VAR <var> URLS <url>... [EXPECTED_HASH <algo=hex>]
#                        [TIMEOUT <s>] [INACTIVITY_TIMEOUT <s>] [RETRIES <n>] [RETRY_BACKOFF <s>] [LOG_TAG <tag>]
#                        [FAILURE_HINT <text>] [ALLOW_FAILURE])
# Downloads <dir>/<name> from the first URL that succeeds, retrying each, and sets <var> to its path. A cached file is
# reused when it matches EXPECTED_HASH; a download that does not is never retried from another URL. When the download
# fails, <var> is "" under ALLOW_FAILURE and configure fails otherwise.
function(qgc_resilient_download)
    # One retry/verify loop over the URLs; splitting it would only pass its state around. file(${_hash_algo}) takes
    # one of the file(<algorithm>) names qgc_parse_expected_hash accepts.
    # cmake-lint: disable=R0912,R0915,C0114
    set(_one_value_keywords
        FILENAME
        DESTINATION_DIR
        RESULT_VAR
        TIMEOUT
        INACTIVITY_TIMEOUT
        RETRIES
        RETRY_BACKOFF
        EXPECTED_HASH
        LOG_TAG
        FAILURE_HINT
    )
    cmake_parse_arguments(PARSE_ARGV 0 ARG "ALLOW_FAILURE" "${_one_value_keywords}" "URLS")

    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_resilient_download: missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_resilient_download: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    foreach(_required FILENAME DESTINATION_DIR RESULT_VAR)
        if(NOT ARG_${_required})
            message(FATAL_ERROR "qgc_resilient_download: ${_required} is required")
        endif()
    endforeach()
    if(NOT ARG_URLS)
        message(FATAL_ERROR "qgc_resilient_download: at least one URL is required")
    endif()
    if(IS_ABSOLUTE "${ARG_FILENAME}"
       OR ARG_FILENAME MATCHES "[/\\\\]"
       OR ARG_FILENAME MATCHES "^[.][.]?$"
    )
        message(FATAL_ERROR "qgc_resilient_download: FILENAME must be a basename")
    endif()
    if(NOT ARG_RESULT_VAR MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "qgc_resilient_download: RESULT_VAR must be a valid variable name")
    endif()

    if(NOT ARG_LOG_TAG)
        set(ARG_LOG_TAG "Download")
    endif()
    if(NOT DEFINED ARG_TIMEOUT)
        if(DEFINED QGC_DOWNLOAD_TIMEOUT)
            set(ARG_TIMEOUT ${QGC_DOWNLOAD_TIMEOUT})
        else()
            set(ARG_TIMEOUT 120)
        endif()
    endif()
    if(NOT DEFINED ARG_INACTIVITY_TIMEOUT)
        if(DEFINED QGC_DOWNLOAD_INACTIVITY_TIMEOUT)
            set(ARG_INACTIVITY_TIMEOUT ${QGC_DOWNLOAD_INACTIVITY_TIMEOUT})
        else()
            set(ARG_INACTIVITY_TIMEOUT 60)
        endif()
    endif()
    if(NOT DEFINED ARG_RETRIES)
        if(DEFINED QGC_DOWNLOAD_RETRIES)
            set(ARG_RETRIES ${QGC_DOWNLOAD_RETRIES})
        else()
            set(ARG_RETRIES 3)
        endif()
    endif()
    if(NOT DEFINED ARG_RETRY_BACKOFF)
        if(DEFINED QGC_DOWNLOAD_RETRY_BACKOFF)
            set(ARG_RETRY_BACKOFF ${QGC_DOWNLOAD_RETRY_BACKOFF})
        else()
            set(ARG_RETRY_BACKOFF 5)
        endif()
    endif()

    foreach(_positive_option IN ITEMS TIMEOUT INACTIVITY_TIMEOUT RETRIES)
        if(NOT "${ARG_${_positive_option}}" MATCHES "^[1-9][0-9]*$")
            message(FATAL_ERROR "qgc_resilient_download: ${_positive_option} must be a positive integer")
        endif()
    endforeach()
    if(NOT ARG_RETRY_BACKOFF MATCHES "^[0-9]+$")
        message(FATAL_ERROR "qgc_resilient_download: RETRY_BACKOFF must be a non-negative integer")
    endif()

    set(_progress_opt SHOW_PROGRESS)
    if(DEFINED ENV{CI})
        set(_progress_opt "")
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
                set(${ARG_RESULT_VAR}
                    "${_dest}"
                    PARENT_SCOPE
                )
                return()
            endif()
        else()
            message(STATUS "${ARG_LOG_TAG}: Using cached ${ARG_FILENAME} (no checksum verification)")
            set(${ARG_RESULT_VAR}
                "${_dest}"
                PARENT_SCOPE
            )
            return()
        endif()
    endif()

    file(MAKE_DIRECTORY "${ARG_DESTINATION_DIR}")
    # Per-process name: configures sharing DESTINATION_DIR must not write the same partial file.
    string(RANDOM LENGTH 12 _tmp_token)
    set(_tmp "${_dest}.${_tmp_token}.tmp")
    set(_tried_urls "")

    foreach(_url IN LISTS ARG_URLS)
        if(NOT _url)
            continue()
        endif()
        set(_attempt 1)
        while(_attempt LESS_EQUAL ARG_RETRIES)
            message(
                STATUS "${ARG_LOG_TAG}: Downloading ${ARG_FILENAME} from ${_url} (attempt ${_attempt}/${ARG_RETRIES})"
            )
            file(
                DOWNLOAD "${_url}" "${_tmp}"
                STATUS _status
                ${_progress_opt}
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
                        file(REMOVE "${_tmp}")
                        if(ARG_ALLOW_FAILURE)
                            # Optional artifact: soft-fail like a network error rather than
                            # abort, but still don't shop mirrors with a pinned checksum.
                            message(WARNING "${ARG_LOG_TAG}: ${_hash_algo} mismatch for ${ARG_FILENAME} from ${_url} "
                                            "(expected ${_expected}, got ${_actual_hash}); skipping (failure allowed)."
                            )
                            set(${ARG_RESULT_VAR}
                                ""
                                PARENT_SCOPE
                            )
                            return()
                        endif()
                        message(FATAL_ERROR "${ARG_LOG_TAG}: ${_hash_algo} mismatch for ${ARG_FILENAME} from ${_url} "
                                            "(expected ${_expected}, got ${_actual_hash}). Refusing to fall back to "
                                            "other mirrors with a pinned checksum."
                        )
                    endif()
                endif()
                file(RENAME "${_tmp}" "${_dest}")
                set(${ARG_RESULT_VAR}
                    "${_dest}"
                    PARENT_SCOPE
                )
                return()
            endif()
            list(GET _status 1 _error)
            file(REMOVE "${_tmp}")
            if(_attempt LESS ARG_RETRIES)
                math(EXPR _backoff "${_attempt} * ${ARG_RETRY_BACKOFF}")
                message(WARNING "${ARG_LOG_TAG}: attempt ${_attempt}/${ARG_RETRIES} failed for ${ARG_FILENAME} from "
                                "${_url}: ${_error}. Retrying in ${_backoff}s..."
                )
                if(_backoff GREATER 0)
                    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep "${_backoff}")
                endif()
            else()
                message(WARNING "${ARG_LOG_TAG}: Failed to download ${ARG_FILENAME} from ${_url} after ${ARG_RETRIES} "
                                "attempts: ${_error}"
                )
            endif()
            math(EXPR _attempt "${_attempt} + 1")
        endwhile()
        list(APPEND _tried_urls "${_url}")
    endforeach()

    if(ARG_ALLOW_FAILURE)
        message(
            STATUS
                "${ARG_LOG_TAG}: All download URLs failed for ${ARG_FILENAME} (allowing failure). Tried: ${_tried_urls}"
        )
        set(${ARG_RESULT_VAR}
            ""
            PARENT_SCOPE
        )
        return()
    endif()

    set(_msg "${ARG_LOG_TAG}: All download URLs failed for ${ARG_FILENAME}.\nTried: ${_tried_urls}")
    if(ARG_FAILURE_HINT)
        string(APPEND _msg "\n${ARG_FAILURE_HINT}")
    endif()
    message(FATAL_ERROR "${_msg}")
endfunction()
