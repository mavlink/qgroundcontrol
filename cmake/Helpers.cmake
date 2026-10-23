# ----------------------------------------------------------------------------
# QGroundControl CMake Helper Functions
# ----------------------------------------------------------------------------

include_guard(GLOBAL)

# ----------------------------------------------------------------------------
# qgc_set_qt_resource_alias
# Sets Qt resource aliases for files based on their filenames
# Args: List of resource files
# ----------------------------------------------------------------------------
function(qgc_set_qt_resource_alias)
    foreach(resource_file IN LISTS ARGN)
        if(NOT EXISTS "${resource_file}")
            message(WARNING "QGC: Resource file does not exist: ${resource_file}")
            continue()
        endif()
        get_filename_component(alias "${resource_file}" NAME)
        set_source_files_properties(
            "${resource_file}"
            PROPERTIES
                QT_RESOURCE_ALIAS "${alias}"
        )
    endforeach()
endfunction()

# ----------------------------------------------------------------------------
# _qgc_write_if_changed
# Writes a generated build-dir file only when its content differs, so the mtime
# stays stable across reconfigures. AUTOMOC re-runs moc for every target when
# its executable (the moccache launcher) is newer than the generated files.
# ----------------------------------------------------------------------------
function(_qgc_write_if_changed path content)
    if(EXISTS "${path}")
        file(READ "${path}" _existing)
        if(_existing STREQUAL content)
            return()
        endif()
    endif()
    file(WRITE "${path}" "${content}")
endfunction()

# ----------------------------------------------------------------------------
# qgc_config_caching
# Configures compiler caching using ccache or sccache if available
# ----------------------------------------------------------------------------
function(qgc_config_caching)
    # cmake-lint: disable=R0915,E1126
    # Reject unusable cache executables found on PATH.
    function(_qgc_verify_cache_tool _ok _path)
        execute_process(
            COMMAND "${_path}" --version
            RESULT_VARIABLE _res
            OUTPUT_QUIET ERROR_QUIET
        )
        if(NOT _res EQUAL 0)
            set(${_ok}
                FALSE
                PARENT_SCOPE
            )
        endif()
    endfunction()

    # Allow presets to prefer a specific backend via QGC_CACHE_BACKEND
    if(QGC_CACHE_BACKEND)
        find_program(QGC_CACHE_PROGRAM NAMES "${QGC_CACHE_BACKEND}" VALIDATOR _qgc_verify_cache_tool)
    endif()
    if(NOT QGC_CACHE_PROGRAM)
        find_program(QGC_CACHE_PROGRAM NAMES ccache sccache VALIDATOR _qgc_verify_cache_tool)
    endif()

    if(QGC_CACHE_PROGRAM)
        get_filename_component(_cache_tool "${QGC_CACHE_PROGRAM}" NAME_WE)
        message(STATUS "QGC: Using ${_cache_tool} (${QGC_CACHE_PROGRAM})")
        string(TOLOWER "${_cache_tool}" _cache_tool)

        if(_cache_tool STREQUAL "ccache")
            set(_ccache_conf "${CMAKE_SOURCE_DIR}/tools/configs/ccache.conf")
            if(CMAKE_HOST_WIN32)
                set(_ccache_wrapper "${CMAKE_BINARY_DIR}/ccache-launcher.cmd")
                configure_file(
                    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ccache-launcher.cmd.in" "${_ccache_wrapper}" @ONLY
                    NEWLINE_STYLE DOS
                )
                set(_cache_launcher "${_ccache_wrapper}")
                # Batch launchers have cmd.exe's 8191-character limit, below Ninja's native Windows limit.
                set(CMAKE_NINJA_FORCE_RESPONSE_FILE
                    ON
                    PARENT_SCOPE
                )
            else()
                # Unix: wrapper script to set env vars at build time.
                # Use defaults so external cache setups (CI/IDE) can override.
                set(_ccache_wrapper "${CMAKE_BINARY_DIR}/ccache-launcher")
                set(_wrapper "#!/bin/sh\n")
                if(EXISTS "${_ccache_conf}")
                    string(APPEND _wrapper "export CCACHE_CONFIGPATH=\"\${CCACHE_CONFIGPATH:-${_ccache_conf}}\"\n")
                endif()
                string(APPEND _wrapper "export CCACHE_DIR=\"\${CCACHE_DIR:-${CMAKE_SOURCE_DIR}/.cache/ccache}\"\n")
                string(APPEND _wrapper "export CCACHE_BASEDIR=\"\${CCACHE_BASEDIR:-${CMAKE_SOURCE_DIR}}\"\n")
                # Delete a header compile's output (the PCH) before regenerating it: ccache otherwise hashes the
                # stale PCH beside the -include'd header as an input, so a regenerated PCH never hits and every
                # object using it misses.
                string(
                    APPEND
                    _wrapper
                    [=[
case " $* " in *" -x c++-header "*|*" -x c-header "*)
    _prev=; for _arg; do [ "$_prev" = -o ] && rm -f "$_arg"; _prev=$_arg; done ;;
esac
]=]
                )
                string(APPEND _wrapper "exec \"${QGC_CACHE_PROGRAM}\" \"$@\"\n")
                _qgc_write_if_changed("${_ccache_wrapper}" "${_wrapper}")
                # cmakelang 0.6.13 does not recognize file(CHMOD) (E1126, disabled for this function).
                file(
                    CHMOD
                    "${_ccache_wrapper}"
                    PERMISSIONS
                    OWNER_READ
                    OWNER_WRITE
                    OWNER_EXECUTE
                    GROUP_READ
                    GROUP_EXECUTE
                    WORLD_READ
                    WORLD_EXECUTE
                )
                set(_cache_launcher "${_ccache_wrapper}")
            endif()
        elseif(_cache_tool STREQUAL "sccache")
            set(_cache_launcher "${QGC_CACHE_PROGRAM}")
            # sccache keys a GCC precompiled header by its preprocessed text, which omits macro definitions, so a
            # changed header can reuse a stale PCH (mozilla/sccache#2562); it cannot cache MSVC PCH users at all.
            # Configure with -DCMAKE_DISABLE_PRECOMPILE_HEADERS=OFF to keep them anyway.
            if(NOT DEFINED CMAKE_DISABLE_PRECOMPILE_HEADERS)
                set(CMAKE_DISABLE_PRECOMPILE_HEADERS
                    ON
                    PARENT_SCOPE
                )
                message(STATUS "QGC: Precompiled headers disabled for sccache")
            endif()
        else()
            return()
        endif()

        set(CMAKE_C_COMPILER_LAUNCHER
            "${_cache_launcher}"
            CACHE STRING "C compiler launcher" FORCE
        )
        set(CMAKE_CXX_COMPILER_LAUNCHER
            "${_cache_launcher}"
            CACHE STRING "CXX compiler launcher" FORCE
        )
        # Linker launchers not currently used but available if needed
        # set(CMAKE_C_LINKER_LAUNCHER "${QGC_CACHE_PROGRAM}" CACHE STRING "C linker cache")
        # set(CMAKE_CXX_LINKER_LAUNCHER "${QGC_CACHE_PROGRAM}" CACHE STRING "CXX linker cache")

        # This runs before project(), so the compiler is only known at generation time. Without the flag, Clang
        # records header mtimes in the PCH and ccache rejects it after a fresh checkout or MAVLink regeneration.
        set(_clang_compile "$<AND:$<COMPILE_LANGUAGE:C,CXX,OBJC,OBJCXX>,$<CXX_COMPILER_ID:AppleClang,Clang>>")
        add_compile_options("$<${_clang_compile}:SHELL:-Xclang -fno-pch-timestamp>")
    else()
        message(WARNING "QGC: No ccache/sccache found - building without a compiler cache")
    endif()
endfunction()

# ----------------------------------------------------------------------------
# _qgc_write_moccache_stats_script
# Writes a post-build script that prints moccache hit/miss stats for the
# ninja build currently running (entries newer than the build start derived
# from .ninja_log, from this build dir). It is a no-op unless MOCCACHE_STATS
# is set in the build environment.
# ----------------------------------------------------------------------------
function(_qgc_write_moccache_stats_script python launcher_py out_var)
    set(_default_dir "${CMAKE_SOURCE_DIR}/.cache/moccache")
    if(CMAKE_HOST_WIN32)
        set(_script "${CMAKE_BINARY_DIR}/moccache-stats.cmd")
        set(_body "@echo off\r\nsetlocal\r\n")
        string(APPEND _body "if not defined MOCCACHE_STATS exit /b 0\r\n")
        string(APPEND _body "if not defined MOCCACHE_DIR set \"MOCCACHE_DIR=${_default_dir}\"\r\n")
        string(APPEND _body "if not defined MOCCACHE_BASEDIR set \"MOCCACHE_BASEDIR=${CMAKE_BINARY_DIR}\"\r\n")
        string(APPEND _body "\"${python}\" \"${launcher_py}\" --show-stats --build-dir \"${CMAKE_BINARY_DIR}\"\r\n")
        string(APPEND _body "exit /b 0\r\n")
        _qgc_write_if_changed("${_script}" "${_body}")
    else()
        set(_script "${CMAKE_BINARY_DIR}/moccache-stats")
        set(_body "#!/bin/sh\n")
        string(APPEND _body "[ -n \"\${MOCCACHE_STATS}\" ] || exit 0\n")
        string(APPEND _body "export MOCCACHE_DIR=\"\${MOCCACHE_DIR:-${_default_dir}}\"\n")
        string(APPEND _body "export MOCCACHE_BASEDIR=\"\${MOCCACHE_BASEDIR:-${CMAKE_BINARY_DIR}}\"\n")
        string(
            APPEND _body "\"${python}\" \"${launcher_py}\" --show-stats --build-dir \"${CMAKE_BINARY_DIR}\" || true\n"
        )
        _qgc_write_if_changed("${_script}" "${_body}")
        # cmakelang 0.6.13 does not recognize file(CHMOD).
        # cmake-lint: disable=E1126
        file(
            CHMOD
            "${_script}"
            PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            OWNER_EXECUTE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE
        )
    endif()
    set(${out_var}
        "${_script}"
        PARENT_SCOPE
    )
endfunction()

# ----------------------------------------------------------------------------
# _qgc_verify_moccache_python
# find_program VALIDATOR: moccache targets Python 3.10 (ruff.toml); an older
# interpreter would fail on every moc invocation, so reject it and let
# qgc_config_moccache fall back to plain moc.
# ----------------------------------------------------------------------------
function(_qgc_verify_moccache_python _ok _path)
    execute_process(
        COMMAND "${_path}" -c "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)"
        RESULT_VARIABLE _res
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT _res EQUAL 0)
        set(${_ok}
            FALSE
            PARENT_SCOPE
        )
    endif()
endfunction()

# ----------------------------------------------------------------------------
# qgc_config_moccache
# Routes AUTOMOC through the tools/moccache package so moc output is cached across
# clean builds. Must be called after find_package(Qt6).
# ----------------------------------------------------------------------------
function(qgc_config_moccache)
    # Keep both platform launchers alongside their shared tool validation.
    # cmakelang 0.6.13 also does not recognize file(CHMOD).
    # cmake-lint: disable=R0915,E1126
    if(DEFINED CMAKE_AUTOMOC_EXECUTABLE AND NOT CMAKE_AUTOMOC_EXECUTABLE STREQUAL "")
        return()
    endif()

    if(CMAKE_HOST_WIN32 AND CMAKE_VERSION VERSION_LESS 3.29)
        message(STATUS "QGC: CMake 3.29 or newer is required for moccache on Windows")
        return()
    endif()

    set(_moccache_core_py "${CMAKE_SOURCE_DIR}/tools/moccache/core.py")
    if(NOT EXISTS "${_moccache_core_py}" OR NOT TARGET Qt6::moc)
        return()
    endif()

    # find_program skips VALIDATOR for a cached result; recheck one left by an older configure.
    if(QGC_MOCCACHE_PYTHON)
        set(_python_ok TRUE)
        _qgc_verify_moccache_python(_python_ok "${QGC_MOCCACHE_PYTHON}")
        if(NOT _python_ok)
            unset(QGC_MOCCACHE_PYTHON CACHE)
        endif()
    endif()
    find_program(QGC_MOCCACHE_PYTHON NAMES python3 python VALIDATOR _qgc_verify_moccache_python)
    if(NOT QGC_MOCCACHE_PYTHON)
        message(STATUS "QGC: python3 >= 3.10 not found - building without moc caching")
        return()
    endif()

    get_target_property(_real_moc Qt6::moc IMPORTED_LOCATION)
    if(NOT _real_moc)
        get_target_property(_real_moc Qt6::moc IMPORTED_LOCATION_RELEASE)
    endif()
    if(NOT _real_moc OR NOT EXISTS "${_real_moc}")
        message(STATUS "QGC: real moc not found - building without moc caching")
        return()
    endif()

    # Importing moccache from a stub lets Python reuse its cached bytecode on every moc run
    set(_launcher_py "${CMAKE_SOURCE_DIR}/tools/moccache_launcher.py")
    if(CMAKE_HOST_WIN32)
        set(_moccache_wrapper "${CMAKE_BINARY_DIR}/moccache-launcher.cmd")
        set(_wrapper "@echo off\r\nsetlocal\r\n")
        string(
            APPEND _wrapper "if not defined MOCCACHE_DIR set \"MOCCACHE_DIR=${CMAKE_SOURCE_DIR}/.cache/moccache\"\r\n"
        )
        string(APPEND _wrapper "if not defined MOCCACHE_BASEDIR set \"MOCCACHE_BASEDIR=${CMAKE_BINARY_DIR}\"\r\n")
        string(APPEND _wrapper "if not defined MOCCACHE_MAX_SIZE set \"MOCCACHE_MAX_SIZE=1G\"\r\n")
        string(APPEND _wrapper "\"${QGC_MOCCACHE_PYTHON}\" \"${_launcher_py}\" --real-moc \"${_real_moc}\" %*\r\n")
        string(APPEND _wrapper "exit /b %ERRORLEVEL%\r\n")
        _qgc_write_if_changed("${_moccache_wrapper}" "${_wrapper}")
    else()
        set(_moccache_wrapper "${CMAKE_BINARY_DIR}/moccache-launcher")
        set(_wrapper "#!/bin/sh\n")
        string(APPEND _wrapper "export MOCCACHE_DIR=\"\${MOCCACHE_DIR:-${CMAKE_SOURCE_DIR}/.cache/moccache}\"\n")
        string(APPEND _wrapper "export MOCCACHE_BASEDIR=\"\${MOCCACHE_BASEDIR:-${CMAKE_BINARY_DIR}}\"\n")
        string(APPEND _wrapper "export MOCCACHE_MAX_SIZE=\"\${MOCCACHE_MAX_SIZE:-1G}\"\n")
        string(
            APPEND _wrapper "exec \"${QGC_MOCCACHE_PYTHON}\" \"${_launcher_py}\" --real-moc \"${_real_moc}\" \"$@\"\n"
        )
        _qgc_write_if_changed("${_moccache_wrapper}" "${_wrapper}")
        file(
            CHMOD
            "${_moccache_wrapper}"
            PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            OWNER_EXECUTE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE
        )
    endif()

    _qgc_write_moccache_stats_script("${QGC_MOCCACHE_PYTHON}" "${_launcher_py}" _moccache_stats)

    set_property(GLOBAL PROPERTY QGC_MOCCACHE_EXECUTABLE "${_moccache_wrapper}")
    set_property(GLOBAL PROPERTY QGC_MOCCACHE_STATS_EXECUTABLE "${_moccache_stats}")
    message(STATUS "QGC: Using moccache for AUTOMOC (${_real_moc})")
endfunction()

# Apply moccache to targets after all AUTOMOC properties have been configured.
function(_qgc_apply_moccache_to_directory directory)
    get_property(_moccache_wrapper GLOBAL PROPERTY QGC_MOCCACHE_EXECUTABLE)
    get_property(
        _targets
        DIRECTORY "${directory}"
        PROPERTY BUILDSYSTEM_TARGETS
    )
    foreach(_target IN LISTS _targets)
        get_target_property(_automoc ${_target} AUTOMOC)
        get_property(
            _automoc_executable_set
            TARGET ${_target}
            PROPERTY AUTOMOC_EXECUTABLE
            SET
        )
        if(_automoc AND NOT _automoc_executable_set)
            set_property(TARGET ${_target} PROPERTY AUTOMOC_EXECUTABLE "${_moccache_wrapper}")
            if(CMAKE_HOST_WIN32)
                # Leave room for the Python launcher below cmd.exe's 8191-character limit.
                set_property(TARGET ${_target} PROPERTY AUTOGEN_COMMAND_LINE_LENGTH_MAX 7000)
            endif()
        endif()
    endforeach()

    get_property(
        _subdirectories
        DIRECTORY "${directory}"
        PROPERTY SUBDIRECTORIES
    )
    foreach(_subdirectory IN LISTS _subdirectories)
        _qgc_apply_moccache_to_directory("${_subdirectory}")
    endforeach()
endfunction()

# Apply the configured moccache launcher to every AUTOMOC target and print
# this build's hit/miss stats after the main executable links (when
# MOCCACHE_STATS is set). Ninja only: the per-build scoping needs .ninja_log.
function(qgc_apply_moccache)
    get_property(_moccache_wrapper GLOBAL PROPERTY QGC_MOCCACHE_EXECUTABLE)
    if(_moccache_wrapper)
        _qgc_apply_moccache_to_directory("${CMAKE_SOURCE_DIR}")
    endif()

    get_property(_moccache_stats GLOBAL PROPERTY QGC_MOCCACHE_STATS_EXECUTABLE)
    if(_moccache_stats
       AND TARGET ${CMAKE_PROJECT_NAME}
       AND CMAKE_GENERATOR MATCHES "Ninja"
    )
        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME}
            POST_BUILD
            COMMAND "${_moccache_stats}"
            COMMENT "moccache stats"
            VERBATIM
        )
    endif()
endfunction()

# ----------------------------------------------------------------------------
# qgc_config_rcc_launcher
# Routes Qt6::rcc through a launcher that:
# - pins the file timestamps rcc embeds in generated resources. Otherwise every
#   fresh checkout changes each qrc_*.cpp and defeats the compiler cache. An
#   exported QT_RCC_SOURCE_DATE_OVERRIDE or SOURCE_DATE_EPOCH still takes precedence.
# - lowers rcc's default zstd level from 19 to 9: about 10x faster for ~2% more
#   resource data. Options a resource passes itself come later and still win.
# Must be called after find_package(Qt6).
# ----------------------------------------------------------------------------
function(qgc_config_rcc_launcher)
    # cmakelang 0.6.13 does not recognize file(CHMOD).
    # cmake-lint: disable=E1126
    if(NOT TARGET Qt6::rcc)
        return()
    endif()

    set(_location_properties IMPORTED_LOCATION)
    get_target_property(_configs Qt6::rcc IMPORTED_CONFIGURATIONS)
    if(_configs)
        foreach(_config IN LISTS _configs)
            list(APPEND _location_properties IMPORTED_LOCATION_${_config})
        endforeach()
    endif()

    if(CMAKE_HOST_WIN32)
        set(_launcher "${CMAKE_BINARY_DIR}/rcc-launcher.cmd")
    else()
        set(_launcher "${CMAKE_BINARY_DIR}/rcc-launcher")
    endif()

    set(_real_rcc "")
    foreach(_property IN LISTS _location_properties)
        get_target_property(_location Qt6::rcc ${_property})
        if(_location
           AND NOT _location STREQUAL _launcher
           AND EXISTS "${_location}"
        )
            set(_real_rcc "${_location}")
            break()
        endif()
    endforeach()
    if(NOT _real_rcc)
        message(STATUS "QGC: rcc not found - rcc launcher not used")
        return()
    endif()

    # rcc treats 0 as unset; any other fixed value keeps its output byte-identical.
    set(_epoch 1)
    # Qt passes --no-zstd, which rejects --compress-algo zstd, when the target QtCore lacks zstd.
    set(_rcc_args "")
    if(QT_FEATURE_zstd)
        set(_rcc_args "--compress-algo zstd --compress 9 ")
    endif()
    if(CMAKE_HOST_WIN32)
        set(_wrapper "@echo off\r\nsetlocal\r\n")
        string(
            APPEND _wrapper
            "if not defined QT_RCC_SOURCE_DATE_OVERRIDE set \"QT_RCC_SOURCE_DATE_OVERRIDE=${_epoch}\"\r\n"
        )
        # Started through cmd.exe without a console window (as by AUTOGEN's generate-time
        # `rcc -h` on CI), rcc shows help and errors in a message box and blocks forever.
        string(APPEND _wrapper "set \"QT_COMMAND_LINE_PARSER_NO_GUI_MESSAGE_BOXES=1\"\r\n")
        string(APPEND _wrapper "\"${_real_rcc}\" ${_rcc_args}%*\r\n")
        string(APPEND _wrapper "exit /b %ERRORLEVEL%\r\n")
        _qgc_write_if_changed("${_launcher}" "${_wrapper}")
    else()
        set(_wrapper "#!/bin/sh\n")
        string(APPEND _wrapper "export QT_RCC_SOURCE_DATE_OVERRIDE=\"\${QT_RCC_SOURCE_DATE_OVERRIDE:-${_epoch}}\"\n")
        string(APPEND _wrapper "exec \"${_real_rcc}\" ${_rcc_args}\"$@\"\n")
        _qgc_write_if_changed("${_launcher}" "${_wrapper}")
        file(
            CHMOD
            "${_launcher}"
            PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            OWNER_EXECUTE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE
        )
    endif()

    foreach(_property IN LISTS _location_properties)
        set_property(TARGET Qt6::rcc PROPERTY ${_property} "${_launcher}")
    endforeach()
    message(STATUS "QGC: Using rcc launcher (${_real_rcc})")
endfunction()

# ----------------------------------------------------------------------------
# qgc_add_big_resources
# Embeds large files with rcc's two-pass mode: rcc writes the payload straight
# into a compiled object instead of the compiler parsing a multi-megabyte byte
# array. Arguments after the resource name are forwarded to qt_add_resources().
# ----------------------------------------------------------------------------
function(qgc_add_big_resources target resource_name)
    qt_add_resources(${target} ${resource_name} BIG_RESOURCES ${ARGN})

    # Qt falls back to ordinary resources where two-pass mode is unsupported (Xcode for iOS).
    set(_object_library ${target}_${resource_name}_obj)
    if(NOT TARGET ${_object_library})
        return()
    endif()

    # rcc patches the payload into the pass-1 object; LTO objects hold compiler IR instead of that placeholder.
    set(_properties INTERPROCEDURAL_OPTIMIZATION)
    foreach(_config IN LISTS CMAKE_CONFIGURATION_TYPES CMAKE_BUILD_TYPE)
        string(TOUPPER "${_config}" _config)
        list(APPEND _properties INTERPROCEDURAL_OPTIMIZATION_${_config})
    endforeach()
    foreach(_property IN LISTS _properties)
        set_property(TARGET ${_object_library} PROPERTY ${_property} OFF)
    endforeach()
    set_property(TARGET ${_object_library} PROPERTY UNITY_BUILD OFF)

    # Only targets in this directory see the rule for the patched object; Qt adds that link for static libraries.
    get_target_property(_target_type ${target} TYPE)
    get_target_property(_target_directory ${target} SOURCE_DIR)
    if(NOT _target_type STREQUAL "STATIC_LIBRARY" AND NOT _target_directory STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        add_custom_target(
            ${target}_${resource_name}_pass2
            DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/.qt/rcc/qrc_${resource_name}${CMAKE_CXX_OUTPUT_EXTENSION}"
            COMMENT "Embedding resource ${resource_name}"
        )
        add_dependencies(${target} ${target}_${resource_name}_pass2)
    endif()
endfunction()

# ----------------------------------------------------------------------------
# qgc_set_linker
# Attempts to use a faster linker (mold, lld, or gold) if available
# Falls back to the system default linker
# ----------------------------------------------------------------------------
function(qgc_set_linker)
    # Cross-compile toolchains supply their own linker. NDK + LTO + mold is broken (NDK ships no LLVMgold.so).
    if(CMAKE_CROSSCOMPILING)
        return()
    endif()

    include(CheckLinkerFlag)
    include(CMakePushCheckState)

    cmake_push_check_state(RESET)

    foreach(_ld mold lld gold)
        set(_flag "-fuse-ld=${_ld}")
        check_linker_flag(CXX "${_flag}" HAVE_LD_${_ld})

        if(HAVE_LD_${_ld})
            cmake_pop_check_state()
            add_link_options("${_flag}")
            set(QGC_LINKER
                "${_ld}"
                PARENT_SCOPE
            )
            message(STATUS "QGC: Using ${_ld} linker")
            return()
        endif()
    endforeach()

    cmake_pop_check_state()

    message(STATUS "QGC: No alternative linker (mold/lld/gold) found - using system default")
endfunction()

# ----------------------------------------------------------------------------
# qgc_set_msvc_linker
# Links MSVC builds with lld-link when QGC_USE_LLD_LINK is ON (CMake 3.29+ LINKER_TYPE).
# lld-link cannot link /GL objects, so IPO (Release) builds keep link.exe.
# ----------------------------------------------------------------------------
function(qgc_set_msvc_linker)
    if(NOT QGC_USE_LLD_LINK OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        return()
    endif()
    if(CMAKE_VERSION VERSION_LESS 3.29)
        message(STATUS "QGC: QGC_USE_LLD_LINK needs CMake 3.29 or newer - using link.exe")
        return()
    endif()
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION OR CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE)
        message(STATUS "QGC: lld-link cannot link IPO (/GL) objects - using link.exe")
        return()
    endif()
    # Use CMake's own lld-link lookup (made at the first configure) so try_compile checks link with the same tool.
    if(NOT IS_ABSOLUTE "${CMAKE_LINKER_LLD}" OR NOT EXISTS "${CMAKE_LINKER_LLD}")
        message(STATUS "QGC: lld-link not found on PATH or in CMAKE_LINKER_LLD - using link.exe")
        return()
    endif()

    set(CMAKE_LINKER_TYPE
        LLD
        PARENT_SCOPE
    )
    set(QGC_LINKER
        lld-link
        PARENT_SCOPE
    )
    message(STATUS "QGC: Using lld-link (${CMAKE_LINKER_LLD})")
endfunction()

# ----------------------------------------------------------------------------
# qgc_enable_pie
# Enables Position Independent Executables (PIE) for improved security
# Note: MSVC/Windows uses ASLR instead of PIE (enabled by default)
# ----------------------------------------------------------------------------
function(qgc_enable_pie)
    if(MSVC)
        return()
    endif()

    include(CheckPIESupported)
    check_pie_supported(OUTPUT_VARIABLE _output LANGUAGES C CXX)

    if(CMAKE_C_LINK_PIE_SUPPORTED AND CMAKE_CXX_LINK_PIE_SUPPORTED)
        set(CMAKE_POSITION_INDEPENDENT_CODE
            ON
            PARENT_SCOPE
        )
        message(STATUS "QGC: PIE enabled")
    else()
        message(WARNING "QGC: PIE not supported - ${_output}")
    endif()
endfunction()

# ----------------------------------------------------------------------------
# qgc_enable_split_dwarf
# Enables -gsplit-dwarf + --gdb-index for the current build.
#
# Split-DWARF emits debug info into sidecar .dwo files instead of embedding
# it in .o; the linker then never reads/relocates DWARF, cutting Debug link
# time by 40-60% on QGC-sized projects. gdb follows the .dwo sidecars
# transparently at debug time.
#
# Only applies on ELF targets (Linux, Android). macOS uses dSYM bundles and
# Windows MSVC uses PDBs — both already separate debug info, so split-DWARF
# is a no-op or actively harmful there. Skipped under IPO/LTO because LTO
# rewrites debug info at link time and defeats the split.
#
# --gdb-index requires gold, lld, or mold; qgc_set_linker() prefers those.
# ----------------------------------------------------------------------------
function(qgc_enable_split_dwarf)
    if(NOT QGC_SPLIT_DWARF)
        return()
    endif()
    if(APPLE
       OR MSVC
       OR NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    )
        return()
    endif()
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        message(STATUS "QGC: split-DWARF disabled (IPO/LTO active)")
        return()
    endif()

    # Per-config so Release (no debug info) is untouched.
    add_compile_options($<$<CONFIG:Debug,RelWithDebInfo>:-gsplit-dwarf>)

    # --gdb-index is a linker feature; only emit when the selected linker
    # supports it. mold/lld/gold do; bfd (system default on older distros)
    # does not. qgc_set_linker() exports QGC_LINKER when it picks one.
    if(QGC_LINKER MATCHES "^(mold|lld|gold)$")
        add_link_options($<$<CONFIG:Debug,RelWithDebInfo>:LINKER:--gdb-index>)
        set(_gdb_index ON)
    else()
        set(_gdb_index OFF)
        message(STATUS "QGC: --gdb-index skipped (needs mold/lld/gold; have '${QGC_LINKER}')")
    endif()

    set(QGC_SPLIT_DWARF_ACTIVE
        ON
        PARENT_SCOPE
    )
    set(QGC_SPLIT_DWARF_GDB_INDEX
        ${_gdb_index}
        PARENT_SCOPE
    )
    message(STATUS "QGC: split-DWARF enabled (-gsplit-dwarf, gdb-index=${_gdb_index})")
endfunction()

# ----------------------------------------------------------------------------
# qgc_enable_ipo
# Enables Interprocedural Optimization (IPO/LTO) for Release builds
# ----------------------------------------------------------------------------
function(qgc_enable_ipo)
    if(LINUX)
        return()
    endif()

    if(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE STREQUAL "Release")
        include(CheckIPOSupported)
        check_ipo_supported(
            RESULT _result
            OUTPUT _output
            LANGUAGES C CXX
        )

        if(_result)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE
                TRUE
                PARENT_SCOPE
            )
            message(STATUS "QGC: IPO/LTO enabled for Release build")
        else()
            message(WARNING "QGC: IPO/LTO not supported - ${_output}")
        endif()
    else()
        message(STATUS "QGC: IPO/LTO disabled for ${CMAKE_BUILD_TYPE} build")
    endif()
endfunction()

# ----------------------------------------------------------------------------
# qgc_require_cpm_added
# Fails configuration if a required CPM package was not added
# Args: package_name - the CPMAddPackage NAME (checks <package_name>_ADDED)
# ----------------------------------------------------------------------------
function(qgc_require_cpm_added package_name)
    if(NOT ARGC EQUAL 1 OR NOT package_name MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "qgc_require_cpm_added: exactly one valid CPM package name is required")
    endif()

    set(_added_variable "${package_name}_ADDED")
    if(NOT DEFINED ${_added_variable} OR NOT "${${_added_variable}}")
        message(
            FATAL_ERROR
                "QGC: ${package_name} (required dependency) was not added by CPM. "
                "This package must be vendored from source; if QGC_USE_SYSTEM_LIBS or "
                "QGC_SYSTEM_LIBS_ONLY resolved it via find_package(), configure without them."
        )
    endif()
endfunction()

# Bundle matching JSON files while keeping paths inside the source directory.
function(qgc_add_json_resources name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "NO_RECURSE" "PREFIX;PATTERN" "")
    if(NOT name OR NOT name MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "qgc_add_json_resources: a valid resource name is required")
    endif()
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_add_json_resources(${name}): missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_add_json_resources(${name}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT TARGET ${CMAKE_PROJECT_NAME})
        message(FATAL_ERROR "qgc_add_json_resources(${name}): project target '${CMAKE_PROJECT_NAME}' does not exist")
    endif()
    if(NOT ARG_PREFIX)
        set(ARG_PREFIX "/json")
    endif()
    if(NOT ARG_PATTERN)
        set(ARG_PATTERN "*.json")
    endif()
    if(IS_ABSOLUTE "${ARG_PATTERN}" OR ARG_PATTERN MATCHES "(^|[/\\\\])[.][.]([/\\\\]|$)")
        message(FATAL_ERROR "qgc_add_json_resources(${name}): PATTERN must remain under the current source directory")
    endif()
    set(_glob GLOB_RECURSE)
    if(ARG_NO_RECURSE)
        set(_glob GLOB)
    endif()
    # Both permitted file() forms are selected immediately above.
    # cmake-lint: disable=C0114
    file(${_glob} _json CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_PATTERN}"
    )
    if(NOT _json)
        message(
            FATAL_ERROR
                "qgc_add_json_resources(${name}): no files matched '${ARG_PATTERN}' in ${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
    qt_add_resources(
        ${CMAKE_PROJECT_NAME} ${name}
        PREFIX "${ARG_PREFIX}"
        FILES
            ${_json}
    )
endfunction()
