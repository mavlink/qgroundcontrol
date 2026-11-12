cmake_minimum_required(VERSION 3.25)

# qgc_git_sparse_checkout() must fetch a pinned commit that is no longer a branch head, check out only the files
# its patterns select, reuse a finished checkout without reaching the repository, and fetch nothing for a package
# with a CPM source override.

include("${CMAKE_CURRENT_LIST_DIR}/../GStreamer/tests/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../modules/GitSparseCheckout.cmake")

find_package(Git QUIET)
if(NOT Git_FOUND)
    message(STATUS "QGC_TEST_SKIPPED: git not found")
    return()
endif()

qgc_test_sandbox(_sandbox qgc_git_sparse_checkout_test)
set(_origin "${_sandbox}/github/acme/params.git")
set(CPM_SOURCE_CACHE "${_sandbox}/cache")

# The module fetches from github.com; an isolated git configuration redirects that to the sandbox.
qgc_test_isolate_git("${_sandbox}/home")
# A Windows path has no leading slash to finish file:///.
string(REGEX REPLACE "^([^/])" "/\\1" _sandbox_url_path "${_sandbox}")
file(WRITE "${_sandbox}/home/.gitconfig"
     "[url \"file://${_sandbox_url_path}/github/\"]\n\tinsteadOf = https://github.com/\n"
)

# Runs git in the origin repository with a fixed identity and no commit signing.
function(_qgc_origin_git)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -c user.name=QGC -c user.email=qgc@example.invalid -c commit.gpgsign=false ${ARGN}
        WORKING_DIRECTORY "${_origin}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "git ${ARGN} failed:\n${_output}${_error}")
    endif()
    set(_qgc_origin_git_output
        "${_output}"
        PARENT_SCOPE
    )
endfunction()

file(WRITE "${_origin}/Copter-4.6/apm.pdef.json" "pinned\n")
file(WRITE "${_origin}/Copter-4.6/Parameters.html" "unused\n")
file(WRITE "${_origin}/README.md" "unused\n")
_qgc_origin_git(init -q)
# Lets a file:// fetch name a commit that is not a branch head, as GitHub does.
_qgc_origin_git(config uploadpack.allowAnySHA1InWant true)
_qgc_origin_git(add -A)
_qgc_origin_git(commit -q --no-verify -m pinned)
_qgc_origin_git(rev-parse HEAD)
set(_pinned "${_qgc_origin_git_output}")
file(WRITE "${_origin}/Copter-4.6/apm.pdef.json" "newer\n")
_qgc_origin_git(commit -q --no-verify -a -m newer)

qgc_git_sparse_checkout(
    NAME Params
    GITHUB_REPOSITORY acme/params
    GIT_TAG ${_pinned}
    PATTERNS "/*/apm.pdef.json"
    SOURCE_DIR_VAR _checkout
)
qgc_test_assert_streq("checkout directory" "${CPM_SOURCE_CACHE}/params/${_pinned}" "${_checkout}")
file(READ "${_checkout}/Copter-4.6/apm.pdef.json" _definitions)
qgc_test_assert_streq("pinned commit's file" "pinned\n" "${_definitions}")
if(EXISTS "${_checkout}/Copter-4.6/Parameters.html" OR EXISTS "${_checkout}/README.md")
    message(FATAL_ERROR "ASSERT sparse checkout: files outside the patterns were checked out")
endif()
qgc_test_pass("pinned commit, selected files only")

# Different patterns for the same commit replace the checkout.
qgc_git_sparse_checkout(
    NAME Params
    GITHUB_REPOSITORY acme/params
    GIT_TAG ${_pinned}
    PATTERNS "/README.md"
    SOURCE_DIR_VAR _checkout
)
if(NOT EXISTS "${_checkout}/README.md" OR EXISTS "${_checkout}/Copter-4.6/apm.pdef.json")
    message(FATAL_ERROR "ASSERT changed patterns: the checkout was not replaced")
endif()
qgc_test_pass("changed patterns")

file(REMOVE_RECURSE "${_origin}")
qgc_git_sparse_checkout(
    NAME Params
    GITHUB_REPOSITORY acme/params
    GIT_TAG ${_pinned}
    PATTERNS "/README.md"
    SOURCE_DIR_VAR _checkout
)
if(NOT EXISTS "${_checkout}/README.md")
    message(FATAL_ERROR "ASSERT reuse: a finished checkout was not kept")
endif()
qgc_test_pass("finished checkout reused offline")

# The origin is gone and this package has no checkout, so a fetch would fail.
set(CPM_Overridden_SOURCE "${_sandbox}/override")
qgc_git_sparse_checkout(
    NAME Overridden
    GITHUB_REPOSITORY acme/params
    GIT_TAG ${_pinned}
    PATTERNS "/README.md"
    SOURCE_DIR_VAR _checkout
)
qgc_test_assert_streq("override directory" "${CPM_Overridden_SOURCE}" "${_checkout}")
if(EXISTS "${CPM_SOURCE_CACHE}/overridden")
    message(FATAL_ERROR "ASSERT override: the source cache was used for a package with a CPM source override")
endif()
qgc_test_pass("CPM source override used without a fetch")
