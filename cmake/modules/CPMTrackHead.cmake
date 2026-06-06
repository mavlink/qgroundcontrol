option(QGC_UPDATE_TRACKED_DEPS "Refresh git deps that track a moving branch/tag HEAD" ON)

function(qgc_track_git_head)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "OFFLINE_OK" "REPO_PATH;GIT_REF;INTERVAL" "")

    if(NOT QGC_UPDATE_TRACKED_DEPS)
        return()
    endif()
    if(NOT ARG_GIT_REF)
        message(FATAL_ERROR "QGC: qgc_track_git_head: GIT_REF is required")
    endif()
    if(NOT ARG_REPO_PATH OR NOT EXISTS "${ARG_REPO_PATH}/.git")
        message(WARNING "QGC: qgc_track_git_head: '${ARG_REPO_PATH}' is not a git checkout; skipping")
        return()
    endif()

    find_package(Git REQUIRED)

    file(LOCK "${ARG_REPO_PATH}/../.qgc-track.lock" GUARD FUNCTION TIMEOUT 300)

    set(_stamp "${ARG_REPO_PATH}/.git/qgc-last-fetch")
    if(ARG_INTERVAL AND EXISTS "${_stamp}")
        file(TIMESTAMP "${_stamp}" _last "%s")
        string(TIMESTAMP _now "%s")
        math(EXPR _delta "${_now} - ${_last}")
        if(_delta LESS ARG_INTERVAL)
            return()
        endif()
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C "${ARG_REPO_PATH}" ls-remote --exit-code origin "${ARG_GIT_REF}"
        RESULT_VARIABLE _ls_rc OUTPUT_VARIABLE _ls_out
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _ls_rc EQUAL 0)
        if(ARG_OFFLINE_OK)
            message(WARNING "QGC: cannot reach origin for '${ARG_GIT_REF}'; keeping cached ${ARG_REPO_PATH}")
            return()
        endif()
        message(FATAL_ERROR "QGC: qgc_track_git_head: ls-remote '${ARG_GIT_REF}' failed for ${ARG_REPO_PATH}")
    endif()
    string(REGEX REPLACE "[ \t].*" "" _remote_sha "${_ls_out}")

    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C "${ARG_REPO_PATH}" rev-parse HEAD
        OUTPUT_VARIABLE _local_sha ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_local_sha STREQUAL _remote_sha)
        file(TOUCH "${_stamp}")
        return()
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} -c gc.auto=0 -C "${ARG_REPO_PATH}" fetch --depth=1 origin "${ARG_GIT_REF}"
        RESULT_VARIABLE _f_rc ERROR_VARIABLE _f_err)
    if(NOT _f_rc EQUAL 0)
        if(ARG_OFFLINE_OK)
            message(WARNING "QGC: fetch failed for ${ARG_REPO_PATH} (${_f_err}); keeping cached")
            return()
        endif()
        message(FATAL_ERROR "QGC: qgc_track_git_head: fetch failed for ${ARG_REPO_PATH}: ${_f_err}")
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C "${ARG_REPO_PATH}" reset --hard FETCH_HEAD
        RESULT_VARIABLE _r_rc ERROR_VARIABLE _r_err)
    if(NOT _r_rc EQUAL 0)
        message(FATAL_ERROR "QGC: qgc_track_git_head: reset --hard failed in ${ARG_REPO_PATH}: ${_r_err}")
    endif()

    file(TOUCH "${_stamp}")
    message(STATUS "QGC: updated ${ARG_REPO_PATH} to ${ARG_GIT_REF} (${_remote_sha})")
endfunction()
