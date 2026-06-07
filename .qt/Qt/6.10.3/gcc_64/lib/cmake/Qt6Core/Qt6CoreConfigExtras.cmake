# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
if(NOT DEFINED QT_DEFAULT_MAJOR_VERSION)
    set(QT_DEFAULT_MAJOR_VERSION 6)
endif()

if (NOT QT_NO_CREATE_TARGETS)
    set(__qt_core_target Qt6::Core)
    get_property(__qt_core_aliased_target TARGET ${__qt_core_target} PROPERTY ALIASED_TARGET)
    if(__qt_core_aliased_target)
        set(__qt_core_target "${__qt_core_aliased_target}")
    endif()
    unset(__qt_core_aliased_target)
    
    set_property(TARGET ${__qt_core_target} APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS $<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG>)
    set_property(TARGET ${__qt_core_target} PROPERTY INTERFACE_COMPILE_FEATURES cxx_decltype)
endif()

# note: all the other AUTOMOC_MACRO_NAMES are already set by upstream CMake
list(APPEND CMAKE_AUTOMOC_MACRO_NAMES Q_GADGET_EXPORT Q_ENUM_NS)
list(REMOVE_DUPLICATES CMAKE_AUTOMOC_MACRO_NAMES)

set(QT6_IS_SHARED_LIBS_BUILD "ON")
set(QT6_DEBUG_POSTFIX "")

set(_Qt6CTestMacros "${CMAKE_CURRENT_LIST_DIR}/Qt6CTestMacros.cmake")

set(QT_VISIBILITY_AVAILABLE TRUE)

_qt_internal_setup_deploy_support()

if(ANDROID_PLATFORM)
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6AndroidMacros.cmake")
    _qt_internal_create_global_android_targets()
    _qt_internal_collect_default_android_abis()
    if(NOT QT_NO_CREATE_TARGETS)
        _qt_internal_add_android_executable_finalizer(${__qt_core_target})
    endif()
endif()

if(QT_FEATURE_permissions AND APPLE)
    if(NOT QT_NO_CREATE_TARGETS)
        set_property(TARGET ${__qt_core_target} APPEND PROPERTY
            INTERFACE_QT_EXECUTABLE_FINALIZERS
                _qt_internal_darwin_permission_finalizer
        )
    endif()
endif()

if(EMSCRIPTEN)
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6WasmMacros.cmake")
endif()

_qt_internal_override_example_install_dir_to_dot()

unset(__qt_core_target)
