cmake_minimum_required(VERSION 3.25)

# Git.cmake must not reconfigure on .git/index (rewritten by every `git status`) but must
# still track HEAD moves, including when the branch ref only lives in packed-refs. It must also
# take the version from version tags only, whatever other tags the commits carry.

include("${CMAKE_CURRENT_LIST_DIR}/../GStreamer/tests/_assert.cmake")

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

# Covers the tags below, which a developer's tag.gpgSign would sign, and the git calls of each Git.cmake configure.
qgc_test_isolate_git("${WORK_DIR}/home")

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
_qgc_git(commit -q --no-verify --date=2001-02-03T04:05:06+00:00 -m initial)
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
# A configure-time date would rewrite the header, and rebuild its users, on every reconfigure.
file(STRINGS "${_build}/qgc_version.h" _date_line REGEX "QGC_APP_DATE")
# Strict ISO 8601 dates print UTC as "Z" in newer Git (2.55 on CI runners) and as "+00:00" in older Git (2.43).
if(NOT _date_line MATCHES "\"2001-02-03T04:05:06(\\+00:00|Z)\"")
    message(FATAL_ERROR "QGC_APP_DATE is not the commit date: ${_date_line}")
endif()
file(TIMESTAMP "${_build}/qgc_version.h" _version_header_mtime "%Y-%m-%dT%H:%M:%S.%f" UTC)

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
file(TIMESTAMP "${_build}/qgc_version.h" _reconfigured_mtime "%Y-%m-%dT%H:%M:%S.%f" UTC)
if(NOT _reconfigured_mtime STREQUAL _version_header_mtime)
    message(FATAL_ERROR "Reconfigure without a HEAD move rewrote qgc_version.h")
endif()

# Reconfigures and requires the versions Git.cmake computes to be *expected*:
# "<describe string> <project version> <commits since the version tag>", the hash as HASH.
function(_qgc_expect_versions expected)
    _qgc_configure(_unused)
    file(READ "${_build}/versions.txt" _versions)
    string(REGEX REPLACE "-g[0-9a-f]+ " "-gHASH " _versions "${_versions}")
    if(NOT _versions STREQUAL expected)
        message(FATAL_ERROR "Expected versions '${expected}' ${ARGN}, got '${_versions}'")
    endif()
endfunction()

# Commits a new file, <name>.txt, to the fixture repo.
function(_qgc_commit name)
    file(WRITE "${_repo}/${name}.txt" "${name}")
    _qgc_git(add -A)
    _qgc_git(commit -q --no-verify -m "${name}")
endfunction()

# master carries an annotated vX.Y.Z-dev marker; daily builds count the commits since it.
_qgc_git(tag -a v9.8.7-dev -m "daily version base")
_qgc_commit(second)
_qgc_expect_versions("v9.8.7-dev-1-gHASH 9.8.7 1" "one commit after the version marker")

# CI's rolling release moves a lightweight `daily` tag along master, so it is on HEAD or on a
# nearer ancestor than the marker. Neither may change a version.
_qgc_git(tag daily)
_qgc_expect_versions("v9.8.7-dev-1-gHASH 9.8.7 1" "with the daily tag on HEAD")
_qgc_commit(third)
_qgc_expect_versions("v9.8.7-dev-2-gHASH 9.8.7 2" "with the daily tag on the parent commit")
_qgc_git(tag -a latest -m "another tag that is not a version")
_qgc_expect_versions("v9.8.7-dev-2-gHASH 9.8.7 2" "with an annotated non-version tag on HEAD")

# A release tag on HEAD still names the version, with or without the v prefix.
_qgc_git(tag -a v9.8.7 -m release)
_qgc_expect_versions("v9.8.7 9.8.7 0" "on a release tag")
_qgc_commit(fourth)
_qgc_git(tag -a 9.9.0 -m "release without the v prefix")
_qgc_expect_versions("9.9.0 9.9.0 0" "on a version tag without the v prefix")
