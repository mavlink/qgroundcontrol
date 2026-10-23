# Reuses configure-check results (check_include_file, check_symbol_exists, check_type_size, ...)
# across fresh build trees. CMake's check commands skip a result variable that is already cached, so
# seeding the results an earlier configure measured with the same toolchain, flags, and salt skips
# their try_compile runs: about 650, mostly libarchive and SDL3, which dominate a fresh configure.
#
# CI sets QGC_CHECK_RESULT_CACHE_DIR inside the restored compiler cache and salts the fingerprint
# with the dependency declarations, runner image, and installed system packages. A developer build
# tree already keeps its results in CMakeCache.txt and does not need this.
#
# Result files are CMake scripts. Like ccache objects, they are trusted only within the cache
# scope that produced them.

include_guard(GLOBAL)

set(QGC_CHECK_RESULT_CACHE_DIR
    ""
    CACHE PATH "Directory of configure-check results reused by fresh configures; empty disables reuse"
)
set(QGC_CHECK_RESULT_CACHE_SALT
    ""
    CACHE STRING "Extra fingerprint input for reused configure-check results, such as dependency versions"
)
mark_as_advanced(QGC_CHECK_RESULT_CACHE_DIR QGC_CHECK_RESULT_CACHE_SALT)

set(_qgc_check_result_cache_schema 1)
# Saving prunes result files that no configure has used for this many days.
set(_qgc_check_result_cache_max_age_days 30)
# Help strings that CMake's Check* modules give the result variables they cache.
string(
    CONCAT _qgc_check_result_help_regex "^(Have (include|includes|function|symbol|library|variable|"
           "correct prototype for) |Test |Result of TEST_BIG_ENDIAN$)"
)

# Hashes everything a check's outcome depends on besides the dependency's own CMake code, which
# QGC_CHECK_RESULT_CACHE_SALT covers.
function(_qgc_check_result_cache_fingerprint out_var)
    set(_configs ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_TRY_COMPILE_CONFIGURATION})
    list(TRANSFORM _configs TOUPPER)
    list(REMOVE_DUPLICATES _configs)

    set(_vars
        CMAKE_SYSTEM_NAME
        CMAKE_SYSTEM_VERSION
        CMAKE_SYSTEM_PROCESSOR
        CMAKE_CROSSCOMPILING
        CMAKE_SYSROOT
        CMAKE_OSX_SYSROOT
        CMAKE_OSX_ARCHITECTURES
        CMAKE_OSX_DEPLOYMENT_TARGET
        ANDROID_ABI
        ANDROID_PLATFORM
        CMAKE_BUILD_TYPE
        CMAKE_CONFIGURATION_TYPES
        CMAKE_TRY_COMPILE_CONFIGURATION
        CMAKE_TRY_COMPILE_TARGET_TYPE
        CMAKE_POSITION_INDEPENDENT_CODE
        CMAKE_MSVC_RUNTIME_LIBRARY
        CMAKE_LINKER_TYPE
        BUILD_SHARED_LIBS
        CMAKE_REQUIRED_FLAGS
        CMAKE_REQUIRED_DEFINITIONS
        CMAKE_REQUIRED_INCLUDES
        CMAKE_REQUIRED_LINK_OPTIONS
        CMAKE_REQUIRED_LIBRARIES
    )
    foreach(_kind IN ITEMS EXE SHARED MODULE STATIC)
        list(APPEND _vars CMAKE_${_kind}_LINKER_FLAGS)
        foreach(_config IN LISTS _configs)
            list(APPEND _vars CMAKE_${_kind}_LINKER_FLAGS_${_config})
        endforeach()
    endforeach()

    get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    list(SORT _languages)
    foreach(_lang IN LISTS _languages)
        foreach(
            _suffix IN
            ITEMS
                COMPILER
                COMPILER_ID
                COMPILER_VERSION
                COMPILER_TARGET
                FLAGS
                STANDARD
                EXTENSIONS
        )
            list(APPEND _vars CMAKE_${_lang}_${_suffix})
        endforeach()
        foreach(_config IN LISTS _configs)
            list(APPEND _vars CMAKE_${_lang}_FLAGS_${_config})
        endforeach()
    endforeach()

    # QGC options choose which dependencies and features configure, and so which checks run.
    get_cmake_property(_cache_vars CACHE_VARIABLES)
    list(FILTER _cache_vars INCLUDE REGEX "^QGC_")
    list(SORT _cache_vars)
    foreach(_var IN LISTS _cache_vars)
        get_property(
            _type
            CACHE "${_var}"
            PROPERTY TYPE
        )
        if(_type STREQUAL "BOOL")
            list(APPEND _vars "${_var}")
        endif()
    endforeach()

    set(_inputs
        "schema=${_qgc_check_result_cache_schema}" "salt=${QGC_CHECK_RESULT_CACHE_SALT}" "cmake=${CMAKE_VERSION}"
        "generator=${CMAKE_GENERATOR}|${CMAKE_GENERATOR_PLATFORM}|${CMAKE_GENERATOR_TOOLSET}"
    )
    foreach(_var IN LISTS _vars)
        list(APPEND _inputs "${_var}=${${_var}}")
    endforeach()
    string(JOIN "\n" _text ${_inputs})
    string(SHA256 _hash "${_text}")
    set(${out_var}
        "${_hash}"
        PARENT_SCOPE
    )
endfunction()

# Called first by a result file: later _qgc_check_result calls apply only for the expected fingerprint.
function(_qgc_check_results_begin fingerprint)
    set(_qgc_check_results_accepted
        FALSE
        PARENT_SCOPE
    )
    if(fingerprint STREQUAL _qgc_check_results_expected)
        set(_qgc_check_results_accepted
            TRUE
            PARENT_SCOPE
        )
    endif()
endfunction()

# Seeds one result unless the variable is already cached (for example, set with -D).
function(_qgc_check_result name value help)
    if(NOT _qgc_check_results_accepted OR DEFINED CACHE{${name}})
        return()
    endif()
    set(${name}
        "${value}"
        CACHE INTERNAL "${help}"
    )
    math(EXPR _seeded "${_qgc_check_results_seeded} + 1")
    set(_qgc_check_results_seeded
        "${_seeded}"
        PARENT_SCOPE
    )
endfunction()

# Seeds the results saved for this fingerprint and saves this configure's results once it completes.
# Call it after the global compiler and linker flags are final and before dependencies are added.
function(qgc_restore_check_results)
    if(NOT QGC_CHECK_RESULT_CACHE_DIR)
        return()
    endif()

    _qgc_check_result_cache_fingerprint(_qgc_check_results_expected)
    set(_file "${QGC_CHECK_RESULT_CACHE_DIR}/${_qgc_check_results_expected}.cmake")
    set_property(GLOBAL PROPERTY _QGC_CHECK_RESULT_CACHE_FILE "${_file}")
    set_property(GLOBAL PROPERTY _QGC_CHECK_RESULT_CACHE_FINGERPRINT "${_qgc_check_results_expected}")
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _qgc_save_check_results)

    if(NOT EXISTS "${_file}")
        message(STATUS "QGC: No saved configure-check results for this toolchain; running the checks")
        return()
    endif()

    set(_qgc_check_results_accepted FALSE)
    set(_qgc_check_results_seeded 0)
    # EVAL rather than include() keeps the shared file out of the build tree's regeneration inputs.
    file(READ "${_file}" _code)
    cmake_language(EVAL CODE "${_code}")
    if(NOT _qgc_check_results_accepted)
        message(STATUS "QGC: Ignoring configure-check results with a different fingerprint: ${_file}")
        return()
    endif()
    # Marks the file as used so pruning keeps it.
    file(TOUCH_NOCREATE "${_file}")
    message(STATUS "QGC: Reused ${_qgc_check_results_seeded} configure-check results from ${_file}")
endfunction()

# Returns the sorted names of the cached check results worth reusing.
function(_qgc_collect_check_results out_var)
    get_cmake_property(_vars CACHE_VARIABLES)
    list(SORT _vars)
    set(_results)
    foreach(_var IN LISTS _vars)
        get_property(
            _type
            CACHE "${_var}"
            PROPERTY TYPE
        )
        if(NOT _type STREQUAL "INTERNAL")
            continue()
        endif()
        get_property(
            _help
            CACHE "${_var}"
            PROPERTY HELPSTRING
        )
        if(_help MATCHES "${_qgc_check_result_help_regex}")
            list(APPEND _results "${_var}")
        elseif(_help MATCHES "^CHECK_TYPE_SIZE: ")
            # A size of 0 marks a multi-architecture build whose sizes differ; the per-architecture
            # map stays in the build tree, so that check must run again. check_type_size skips only
            # when its result variable (HAVE_<var> by default) is cached too.
            if("$CACHE{${_var}}" STREQUAL "0")
                continue()
            endif()
            list(APPEND _results "${_var}")
            get_property(
                _have_help
                CACHE "HAVE_${_var}"
                PROPERTY HELPSTRING
            )
            if(_have_help STREQUAL "Result of TRY_COMPILE")
                list(APPEND _results "HAVE_${_var}")
            endif()
        endif()
    endforeach()
    set(${out_var}
        "${_results}"
        PARENT_SCOPE
    )
endfunction()

# Renders the named results as the script qgc_restore_check_results evaluates.
function(_qgc_format_check_results out_content out_count fingerprint)
    set(_content "# Configure-check results saved by cmake/modules/CheckResultCache.cmake.\n")
    string(APPEND _content "_qgc_check_results_begin([==[${fingerprint}]==])\n")
    set(_count 0)
    foreach(_var IN LISTS ARGN)
        set(_value "$CACHE{${_var}}")
        get_property(
            _help
            CACHE "${_var}"
            PROPERTY HELPSTRING
        )
        # Bracket arguments are literal; skip the rare entry they cannot quote on one line.
        set(_quotable TRUE)
        foreach(_field IN ITEMS _var _value _help)
            string(FIND "${${_field}}" "]=" _bracket)
            string(FIND "${${_field}}" "\n" _newline)
            if(_bracket GREATER -1 OR _newline GREATER -1)
                set(_quotable FALSE)
            endif()
        endforeach()
        if(_quotable)
            string(APPEND _content "_qgc_check_result([==[${_var}]==] [==[${_value}]==] [==[${_help}]==])\n")
            math(EXPR _count "${_count} + 1")
        endif()
    endforeach()
    set(${out_content}
        "${_content}"
        PARENT_SCOPE
    )
    set(${out_count}
        "${_count}"
        PARENT_SCOPE
    )
endfunction()

# Removes result files in dir, other than keep, that no configure has used recently.
function(_qgc_prune_check_results dir keep)
    file(GLOB _saved "${dir}/*.cmake")
    string(TIMESTAMP _now "%s" UTC)
    math(EXPR _max_age "${_qgc_check_result_cache_max_age_days} * 86400")
    foreach(_other IN LISTS _saved)
        if(_other STREQUAL keep)
            continue()
        endif()
        file(TIMESTAMP "${_other}" _modified "%s" UTC)
        math(EXPR _age "${_now} - ${_modified}")
        if(_age GREATER _max_age)
            file(REMOVE "${_other}")
        endif()
    endforeach()
endfunction()

# Deferred to the end of a successful configure; a failed configure saves nothing.
function(_qgc_save_check_results)
    get_property(_file GLOBAL PROPERTY _QGC_CHECK_RESULT_CACHE_FILE)
    get_property(_fingerprint GLOBAL PROPERTY _QGC_CHECK_RESULT_CACHE_FINGERPRINT)

    _qgc_collect_check_results(_results)
    _qgc_format_check_results(_content _count "${_fingerprint}" ${_results})

    set(_existing "")
    if(EXISTS "${_file}")
        file(READ "${_file}" _existing)
    endif()
    if(NOT _existing STREQUAL _content)
        file(WRITE "${_file}.tmp" "${_content}")
        file(RENAME "${_file}.tmp" "${_file}")
        message(STATUS "QGC: Saved ${_count} configure-check results to ${_file}")
    endif()

    get_filename_component(_dir "${_file}" DIRECTORY)
    _qgc_prune_check_results("${_dir}" "${_file}")
endfunction()
