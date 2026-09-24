cmake_minimum_required(VERSION 3.25)

# Git.cmake must not reconfigure on .git/index (rewritten by every `git status`) but must
# still track HEAD moves, including when the branch ref only lives in packed-refs.

foreach(_required IN ITEMS QGC_MODULE_DIR FIXTURE_SOURCE_DIR WORK_DIR)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

find_program(_qgc_git NAMES git)
if(NOT _qgc_git)
    message(STATUS "QGC_TEST_SKIPPED: git not found")
    return()
endif()

set(_repo "${WORK_DIR}/repo")
set(_build "${WORK_DIR}/build")
file(REMOVE_RECURSE "${WORK_DIR}")
file(COPY "${FIXTURE_SOURCE_DIR}/" DESTINATION "${_repo}")

# Runs git in the fixture repo with a fixed identity and no commit signing.
function(_qgc_git)
    execute_process(
        COMMAND "${_qgc_git}" -c user.name=QGC -c user.email=qgc@example.invalid -c commit.gpgsign=false ${ARGN}
        WORKING_DIRECTORY "${_repo}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "git ${ARGN} failed:\n${_output}${_error}")
    endif()
endfunction()

# Configures the fixture repo and returns its CMAKE_CONFIGURE_DEPENDS with symlinks resolved.
function(_qgc_configure out_var)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" ${ARGN} -S "${_repo}" -B "${_build}" "-DQGC_MODULE_DIR=${QGC_MODULE_DIR}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Git fixture configure failed:\n${_output}${_error}")
    endif()
    file(READ "${_build}/configure-depends.txt" _depends)
    set(_real_depends "")
    foreach(_depend IN LISTS _depends)
        get_filename_component(_real "${_depend}" REALPATH)
        list(APPEND _real_depends "${_real}")
    endforeach()
    set(${out_var}
        "${_real_depends}"
        PARENT_SCOPE
    )
endfunction()

_qgc_git(init -q)
_qgc_git(symbolic-ref HEAD refs/heads/qgc-test)
_qgc_git(add -A)
_qgc_git(commit -q --no-verify -m initial)
get_filename_component(_git_dir "${_repo}/.git" REALPATH)

_qgc_configure(_depends --fresh)
if(NOT "${_git_dir}/logs/HEAD" IN_LIST _depends)
    message(FATAL_ERROR "logs/HEAD is not a configure dependency: ${_depends}")
endif()
if("${_git_dir}/index" IN_LIST _depends)
    message(FATAL_ERROR ".git/index must not be a configure dependency: ${_depends}")
endif()
if(NOT "${_git_dir}/refs/heads/qgc-test" IN_LIST _depends)
    message(FATAL_ERROR "Loose branch ref is not a configure dependency: ${_depends}")
endif()

_qgc_git(pack-refs --all)
if(EXISTS "${_git_dir}/refs/heads/qgc-test")
    message(FATAL_ERROR "git pack-refs left the loose branch ref in place")
endif()

_qgc_configure(_depends)
if(NOT "${_git_dir}/logs/HEAD" IN_LIST _depends)
    message(FATAL_ERROR "logs/HEAD is not a configure dependency with packed refs: ${_depends}")
endif()
if(NOT "${_git_dir}/packed-refs" IN_LIST _depends)
    message(FATAL_ERROR "packed-refs is not a configure dependency: ${_depends}")
endif()
