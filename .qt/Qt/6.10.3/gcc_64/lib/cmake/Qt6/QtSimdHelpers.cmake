# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Handle files that need special SIMD-related flags.
#
# This function adds the passed source files to the given target only if the SIMD specific condition
# evaluates to true. It also adds the SIMD architecture specific flags as compile options to those
# source files.
#
# Arguments
# NAME: is deprecated, don't use it.
# SIMD: name of the simd architecture, e.g. sse2, avx, neon. A Qt feature named QT_FEATURE_foo
#       should exist, as well as QT_CFLAGS_foo assignment in QtCompilerOptimization.cmake.
# COMPILE_FLAGS: extra compile flags to set for the passed source files
# EXCLUDE_OSX_ARCHITECTURES: On apple platforms, specifies which architectures should not get the
#                            SIMD compiler flags. This is mostly relevant for fat / universal builds
#
function(qt_internal_add_simd_part target)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        ""
        "NAME;SIMD"
        "${__default_private_args};COMPILE_FLAGS;EXCLUDE_OSX_ARCHITECTURES")
    _qt_internal_validate_all_args_are_parsed(arg)

    if ("x${arg_SIMD}" STREQUAL x)
        message(FATAL_ERROR "qt_add_simd_part needs a SIMD type to be set.")
    endif()

    set(condition "QT_FEATURE_${arg_SIMD}")
    string(TOUPPER "QT_CFLAGS_${arg_SIMD}" simd_flags_var_name)
    set(simd_flags_expanded "")

    # As per mkspecs/features/simd.prf, the arch_haswell SIMD compiler is enabled when
    # qmake's CONFIG contains "avx2", which maps to CMake's QT_FEATURE_avx2.
    # The list of dependencies 'avx2 bmi bmi2 f16c fma lzcnt popcnt' only influences whether
    # the 'arch_haswell' SIMD flags need to be added explicitly to the compiler invocation.
    # If the compiler adds them implicitly, they must be present in qmake's QT_CPU_FEATURES as
    # detected by the architecture test, and thus they are present in TEST_subarch_result.
    if("${arg_SIMD}" STREQUAL arch_haswell)
        set(condition "QT_FEATURE_avx2")

        # Use avx2 flags as per simd.prf, if there are no specific arch_haswell flags specified in
        # QtCompilerOptimization.cmake.
        if("${simd_flags_var_name}" STREQUAL "")
            set(simd_flags_var_name "QT_CFLAGS_AVX2")
        endif()

    # The avx512 profiles dependencies DO influence if the SIMD compiler will be executed,
    # so each of the profile dependencies have to be in qmake's CONFIG for the compiler to be
    # enabled, which means the CMake features have to evaluate to true.
    # Also the profile flags to be used are a combination of arch_haswell, avx512f and each of the
    # dependencies.
    elseif("${arg_SIMD}" STREQUAL avx512common)
        set(condition "QT_FEATURE_avx512cd")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_ARCH_HASWELL}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512F}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512CD}")
        list(REMOVE_DUPLICATES simd_flags_expanded)
    elseif("${arg_SIMD}" STREQUAL avx512core)
        set(condition "QT_FEATURE_avx512cd AND QT_FEATURE_avx512bw AND QT_FEATURE_avx512dq AND QT_FEATURE_avx512vl")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_ARCH_HASWELL}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512F}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512CD}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512BW}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512DQ}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512VL}")
        list(REMOVE_DUPLICATES simd_flags_expanded)
    endif()

    qt_evaluate_config_expression(result ${condition})
    if(${result})
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_add_simd_part(${target} SIMD ${arg_SIMD} ...): Evaluated")
        endif()

        if(NOT simd_flags_expanded)
            set(simd_flags_expanded "${${simd_flags_var_name}}")
        endif()

        # Only process OSX_ARCHITECTURES when targeting Apple platforms, otherwise it might fail
        # non-Apple builds when CMAKE_OSX_ARCHITECTURES is accidentally passed to configure.
        if(APPLE)
            # If requested, don't pass the simd specific flags to excluded arches on apple platforms.
            # Mostly important for universal / fat builds.
            get_target_property(osx_architectures ${target} OSX_ARCHITECTURES)
            if(simd_flags_expanded AND osx_architectures AND arg_EXCLUDE_OSX_ARCHITECTURES)
                list(REMOVE_ITEM osx_architectures ${arg_EXCLUDE_OSX_ARCHITECTURES})

                # Assumes that simd_flags_expanded contains only one item on apple platforms.
                list(TRANSFORM osx_architectures
                     REPLACE "^(.+)$" "-Xarch_\\1;${simd_flags_expanded}"
                     OUTPUT_VARIABLE simd_flags_expanded)
            endif()
        endif()

        # The manual loop is done on purpose. Check Gerrit comments for
        # 1b7008a3d784f3f266368f824cb43d473a301ba1.
        foreach(source IN LISTS arg_SOURCES)
            set_property(SOURCE "${source}" APPEND
                PROPERTY COMPILE_OPTIONS
                ${simd_flags_expanded}
                ${arg_COMPILE_FLAGS}
            )
        endforeach()
        set_source_files_properties(${arg_SOURCES} PROPERTIES
                                                   SKIP_PRECOMPILE_HEADERS TRUE
                                                   SKIP_UNITY_BUILD_INCLUSION TRUE)
        target_sources(${target} PRIVATE ${arg_SOURCES})
    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_add_simd_part(${target} SIMD ${arg_SIMD} ...): Skipped")
        endif()
    endif()
endfunction()
