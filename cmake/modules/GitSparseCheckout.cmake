include_guard(GLOBAL)

# Runs git in <directory> and fails configure with git's diagnostics when it fails.
function(_qgc_sparse_checkout_git directory)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${directory}" ${ARGN}
        RESULT_VARIABLE _result
        ERROR_VARIABLE _error
        OUTPUT_QUIET
    )
    if(NOT _result EQUAL 0)
        list(JOIN ARGN " " _command)
        message(FATAL_ERROR "QGC: 'git ${_command}' failed in ${directory}:\n${_error}")
    endif()
endfunction()

# Fetches <commit> alone from the origin of the repository in <directory>, in up to three attempts, and fails
# configure with git's diagnostics when none succeeds. <repository> names the origin in messages.
function(_qgc_sparse_checkout_fetch directory repository commit)
    message(STATUS "QGC: Fetching ${repository} at ${commit}")
    foreach(_attempt RANGE 1 3 1)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${directory}" fetch --quiet --depth=1 origin "${commit}"
            RESULT_VARIABLE _result
            ERROR_VARIABLE _error
        )
        if(_result EQUAL 0)
            return()
        endif()
        if(_attempt LESS 3)
            # The linear backoff of qgc_resilient_download().
            math(EXPR _backoff "${_attempt} * 5")
            message(STATUS "QGC: Fetch attempt ${_attempt}/3 failed, retrying in ${_backoff}s")
            execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep "${_backoff}")
        endif()
    endforeach()
    message(FATAL_ERROR "QGC: Cannot fetch ${repository} at ${commit}:\n${_error}")
endfunction()

# qgc_git_sparse_checkout(NAME <name> GITHUB_REPOSITORY <owner/name> GIT_TAG <commit> PATTERNS <pattern>...
#                         SOURCE_DIR_VAR <var>)
# Checks out the files of <commit> that match the sparse-checkout PATTERNS (gitignore syntax) and sets <var> to
# their directory. Only that commit is fetched, so GIT_TAG must be a full commit hash. The directory is named for
# the commit, under the CPM source cache, and a complete checkout is reused without network access. Nothing is
# fetched when CPM_<name>_SOURCE is set: <var> is that directory, which CPMAddPackage(NAME <name>) uses as well.
function(qgc_git_sparse_checkout)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "NAME;GITHUB_REPOSITORY;GIT_TAG;SOURCE_DIR_VAR" "PATTERNS")
    if(ARG_KEYWORDS_MISSING_VALUES OR ARG_UNPARSED_ARGUMENTS)
        message(
            FATAL_ERROR "qgc_git_sparse_checkout: malformed arguments "
                        "(missing values: '${ARG_KEYWORDS_MISSING_VALUES}', unknown: '${ARG_UNPARSED_ARGUMENTS}')"
        )
    endif()
    foreach(_required IN ITEMS NAME GITHUB_REPOSITORY GIT_TAG SOURCE_DIR_VAR PATTERNS)
        if(NOT ARG_${_required})
            message(FATAL_ERROR "qgc_git_sparse_checkout: ${_required} is required")
        endif()
    endforeach()
    string(LENGTH "${ARG_GIT_TAG}" _commit_length)
    if(NOT ARG_GIT_TAG MATCHES "^[0-9a-f]+$" OR NOT _commit_length EQUAL 40)
        message(FATAL_ERROR "qgc_git_sparse_checkout: GIT_TAG must be a full commit hash, got '${ARG_GIT_TAG}'")
    endif()

    # CPM's local source override: CPMAddPackage() would discard a checkout made here.
    if(NOT "${CPM_${ARG_NAME}_SOURCE}" STREQUAL "")
        set(${ARG_SOURCE_DIR_VAR}
            "${CPM_${ARG_NAME}_SOURCE}"
            PARENT_SCOPE
        )
        return()
    endif()

    if(CPM_SOURCE_CACHE)
        set(_cache "${CPM_SOURCE_CACHE}")
    else()
        set(_cache "${CMAKE_BINARY_DIR}/_deps")
    endif()
    string(TOLOWER "${ARG_NAME}" _name)
    set(_directory "${_cache}/${_name}/${ARG_GIT_TAG}")
    set(${ARG_SOURCE_DIR_VAR}
        "${_directory}"
        PARENT_SCOPE
    )

    # The stamp records what a finished checkout holds; an interrupted one has none and starts over.
    list(JOIN ARG_PATTERNS "\n" _patterns)
    set(_stamp "${_directory}/.git/qgc-sparse-checkout")
    set(_state "${ARG_GIT_TAG}\n${_patterns}\n")

    file(MAKE_DIRECTORY "${_cache}/${_name}")
    file(
        LOCK "${_directory}.lock"
        GUARD FUNCTION
        TIMEOUT 600
    )
    if(EXISTS "${_stamp}")
        file(READ "${_stamp}" _checked_out)
        if(_checked_out STREQUAL _state)
            return()
        endif()
    endif()

    find_package(Git REQUIRED)
    file(REMOVE_RECURSE "${_directory}")
    _qgc_sparse_checkout_git("${_cache}/${_name}" init --quiet "${ARG_GIT_TAG}")
    _qgc_sparse_checkout_git("${_directory}" remote add origin "https://github.com/${ARG_GITHUB_REPOSITORY}.git")
    _qgc_sparse_checkout_git("${_directory}" config core.sparseCheckout true)
    file(WRITE "${_directory}/.git/info/sparse-checkout" "${_patterns}\n")
    _qgc_sparse_checkout_fetch("${_directory}" "${ARG_GITHUB_REPOSITORY}" "${ARG_GIT_TAG}")
    _qgc_sparse_checkout_git(
        "${_directory}"
        -c
        advice.detachedHead=false
        checkout
        --quiet
        --detach
        FETCH_HEAD
    )
    file(WRITE "${_stamp}" "${_state}")
endfunction()
