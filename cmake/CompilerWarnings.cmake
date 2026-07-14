# ----------------------------------------------------------------------------
# QGroundControl Compiler Warnings Configuration
# Sets warning levels and treats warnings as errors for QGC source code
# while allowing more lenient settings for third-party dependencies
# ----------------------------------------------------------------------------

include_guard(GLOBAL)

include(CheckCompilerFlag)
include(CMakePushCheckState)

function(qgc_add_optional_no_warning target scope warning)
    if(NOT target OR NOT TARGET ${target})
        message(FATAL_ERROR "QGC: qgc_add_optional_no_warning: Target '${target}' does not exist")
    endif()
    string(MAKE_C_IDENTIFIER "QGC_HAVE_W_${warning}" _var)
    cmake_push_check_state(RESET)
    check_compiler_flag(CXX "-W${warning}" ${_var})
    cmake_pop_check_state()
    if(${_var})
        target_compile_options(${target} ${scope} "-Wno-${warning}")
    endif()
endfunction()

# ----------------------------------------------------------------------------
# QGC Warning Flags for Main Source Code
# ----------------------------------------------------------------------------
function(qgc_set_warning_flags target)
    if(NOT target OR NOT TARGET ${target})
        message(FATAL_ERROR "QGC: qgc_set_warning_flags: Target '${target}' does not exist")
    endif()

    set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ${QGC_ENABLE_WERROR})

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE
            -Wall                       # Enable common warnings
            -Wextra                     # Enable extra warnings
            -Wshadow                    # Warn about variable shadowing
            #-Wnon-virtual-dtor          # Warn about non-virtual destructors
            #-Wold-style-cast            # Warn about C-style casts
            #-Wcast-align                # Warn about pointer cast alignment
            #-Wunused                    # Warn about unused entities
            #-Woverloaded-virtual        # Warn about overloaded virtual functions
            #-Wsign-conversion           # Warn about sign conversions
            #-Wmisleading-indentation    # Warn about misleading indentation
            #-Wduplicated-cond           # Warn about duplicated conditions (GCC/Clang 14+)
            #-Wduplicated-branches       # Warn about duplicated branches (GCC/Clang 14+)
            #-Wnull-dereference          # Warn about null pointer dereferences
            #-Wformat=2                  # Strict format string checking
            #-Wconversion                # Warn about implicit conversions
            #-Wdouble-promotion          # Warn about float to double promotion
            #-Wswitch-enum               # Warn about missing enum cases in switch

            # The following warnings are temporarily disabled due to known issues in QGC codebase that need to be addressed
            -Wno-switch

            # Qt-specific warnings to disable
            -Wno-unknown-warning-option           # Don't error on unknown warning options
        )
        qgc_add_optional_no_warning(${target} PRIVATE deprecated-enum-enum-conversion)

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -Wall                       # Enable common warnings
            -Wextra                     # Enable extra warnings
            -Wshadow                    # Warn about variable shadowing
            #-Wnon-virtual-dtor          # Warn about non-virtual destructors
            #-Wold-style-cast            # Warn about C-style casts
            #-Wcast-align                # Warn about pointer cast alignment
            #-Wunused                    # Warn about unused entities
            #-Woverloaded-virtual        # Warn about overloaded virtual functions
            #-Wsign-conversion           # Warn about sign conversions
            #-Wmisleading-indentation    # Warn about misleading indentation
            #-Wduplicated-cond           # Warn about duplicated conditions
            #-Wduplicated-branches       # Warn about duplicated branches
            #-Wlogical-op                # Warn about logical operations
            #-Wnull-dereference          # Warn about null pointer dereferences
            #-Wuseless-cast              # Warn about useless casts
            #-Wformat=2                  # Strict format string checking
            #-Wdouble-promotion          # Warn about float to double promotion
            #-Wconversion                # Warn about implicit conversions
            #-Wswitch-enum               # Warn about missing enum cases in switch
        )

    elseif(MSVC)
        target_compile_options(${target} PRIVATE
            /W2                         # Warning level 2 (ultimate target is level 3)

            # The following warnings are temporarily disabled due to known issues in QGC codebase that need to be addressed
            /wd4996                     # deprecated functions (strncpy, etc)
            /wd4389                     # signed/unsigned mismatch

            /wd4068                     # unknown pragma (for clang pragmas)
            /wd4127                     # conditional expression is constant (Qt macros)
            /wd4251                     # needs to have dll-interface (Qt classes)
            /wd4275                     # non dll-interface class used as base (Qt classes)
            /wd4819                     # character encoding issues (non-issue for UTF-8)
        )
    endif()
endfunction()

# ----------------------------------------------------------------------------
# Lenient Warning Settings for Third-Party Dependencies (CPM packages)
#
# Use this function to suppress warnings for compiled (STATIC/SHARED) third-party
# library targets. Call it after CPMAddPackage() with the target name.
#
# DO NOT use this for header-only (INTERFACE) libraries. Because INTERFACE compile
# options propagate to all consuming targets, calling this on an INTERFACE library
# would silently suppress ALL warnings in QGC's own source code.
#
# For header-only dependencies, use SYSTEM include directories instead:
#   target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE ${dep_SOURCE_DIR}/include)
# This tells the compiler to treat those headers as system headers, suppressing
# warnings within them without affecting QGC source code.
# ----------------------------------------------------------------------------
function(qgc_disable_dependency_warnings target)
    if(NOT target OR NOT TARGET ${target})
        message(WARNING "QGC: qgc_disable_dependency_warnings: Target '${target}' does not exist")
        return()
    endif()

    get_target_property(_target_type ${target} TYPE)
    if(_target_type STREQUAL "INTERFACE_LIBRARY")
        message(WARNING "QGC: qgc_disable_dependency_warnings: Target '${target}' is an INTERFACE library. "
            "Warning flags would propagate to all consumers. Use SYSTEM include directories instead.")
        return()
    endif()

    set(_scope PRIVATE)

    set_target_properties(${target} PROPERTIES
        COMPILE_WARNING_AS_ERROR OFF
        QGC_DEPENDENCY_TARGET TRUE
        SYSTEM TRUE
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} ${_scope}
            -w                          # Suppress all warnings
            -Wno-error                  # Don't treat warnings as errors
            -Wno-unknown-warning-option # Don't warn about unknown warning options
        )

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} ${_scope}
            -w                          # Suppress all warnings
            -Wno-error                  # Don't treat warnings as errors
        )

    elseif(MSVC)
        target_compile_options(${target} ${_scope}
            /W0                         # Disable all warnings
            /WX-                        # Don't treat warnings as errors
        )
    endif()
endfunction()

# Apply project-owned compile policy exactly once to a compiled target. Coverage
# and sanitizer modules are optional, so their hooks are discovered at call time.
function(qgc_configure_target target)
    if(NOT target OR NOT TARGET ${target})
        message(FATAL_ERROR "QGC: qgc_configure_target: Target '${target}' does not exist")
    endif()

    get_target_property(_imported ${target} IMPORTED)
    get_target_property(_target_type ${target} TYPE)
    if(_imported OR _target_type STREQUAL "INTERFACE_LIBRARY" OR _target_type STREQUAL "UTILITY")
        message(FATAL_ERROR
            "QGC: qgc_configure_target: '${target}' must be a non-imported compiled target")
    endif()

    get_target_property(_configured ${target} QGC_PROJECT_TARGET_CONFIGURED)
    if(_configured)
        return()
    endif()

    qgc_set_warning_flags(${target})
    if(COMMAND qgc_apply_coverage_to_target)
        qgc_apply_coverage_to_target(${target})
    endif()
    if(COMMAND qgc_apply_sanitizers_to_target)
        qgc_apply_sanitizers_to_target(${target})
    endif()
    set_target_properties(${target} PROPERTIES QGC_PROJECT_TARGET_CONFIGURED TRUE)
endfunction()

function(_qgc_configure_directory_targets directory root_directory)
    get_property(_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_target IN LISTS _targets)
        get_target_property(_dependency_target ${_target} QGC_DEPENDENCY_TARGET)
        get_target_property(_target_type ${_target} TYPE)
        if(NOT _dependency_target AND _target_type MATCHES
           "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$"
        )
            qgc_configure_target(${_target})
        endif()
    endforeach()

    get_property(_subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(_subdirectory IN LISTS _subdirectories)
        get_property(_source_directory DIRECTORY "${_subdirectory}" PROPERTY SOURCE_DIR)
        cmake_path(IS_PREFIX root_directory "${_source_directory}" NORMALIZE _is_project_directory)
        if(_is_project_directory)
            _qgc_configure_directory_targets("${_subdirectory}" "${root_directory}")
        endif()
    endforeach()
endfunction()

# Configure all project-owned compiled targets created below a source directory.
# External add_subdirectory() trees are excluded even when introduced by a QGC
# CMakeLists file, and qgc_disable_dependency_warnings() marks in-tree vendored
# targets so they are not reconfigured as project code.
function(qgc_configure_directory_targets directory)
    if(NOT IS_ABSOLUTE "${directory}" OR NOT IS_DIRECTORY "${directory}")
        message(FATAL_ERROR
            "QGC: qgc_configure_directory_targets: '${directory}' must be an absolute source directory")
    endif()

    get_property(_source_directory DIRECTORY "${directory}" PROPERTY SOURCE_DIR)
    if(NOT _source_directory)
        message(FATAL_ERROR
            "QGC: qgc_configure_directory_targets: '${directory}' has not been added to the build")
    endif()
    _qgc_configure_directory_targets("${directory}" "${_source_directory}")
endfunction()

# ----------------------------------------------------------------------------
# Apply warning flags to QGC main target
# Called from main CMakeLists.txt after target is created
# ----------------------------------------------------------------------------
function(qgc_apply_warning_flags)
    if(QGC_ENABLE_WERROR)
        message(STATUS "QGC: Enabling warnings as errors for main source code")
    else()
        message(STATUS "QGC: Compiler warnings enabled without warnings-as-errors")
    endif()
    qgc_configure_target(${CMAKE_PROJECT_NAME})
endfunction()
