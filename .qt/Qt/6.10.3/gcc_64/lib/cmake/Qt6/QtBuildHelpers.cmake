# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_validate_cmake_generator)
    get_property(warning_shown GLOBAL PROPERTY _qt_validate_cmake_generator_warning_shown)

    if(NOT warning_shown
            AND NOT CMAKE_GENERATOR MATCHES "Ninja"
            AND NOT (IOS AND QT_INTERNAL_IS_STANDALONE_TEST)
            AND NOT QT_SILENCE_CMAKE_GENERATOR_WARNING
            AND NOT DEFINED ENV{QT_SILENCE_CMAKE_GENERATOR_WARNING})
        set_property(GLOBAL PROPERTY _qt_validate_cmake_generator_warning_shown TRUE)
        message(WARNING
               "The officially supported CMake generator for building Qt is "
               "Ninja / Ninja Multi-Config. "
               "You are using: '${CMAKE_GENERATOR}' instead. "
               "Thus, you might encounter issues. Use at your own risk.")
    endif()
endfunction()

macro(qt_internal_set_qt_building_qt)
    # Set the QT_BUILDING_QT variable so we can verify whether we are building
    # Qt from source.
    # Make sure not to set it when building a standalone test, otherwise
    # upon reconfiguration we get an error about qt_internal_add_test
    # not being found due the if(NOT QT_BUILDING_QT) check we have
    # in each standalone test.
    if(NOT QT_INTERNAL_IS_STANDALONE_TEST)
        set(QT_BUILDING_QT TRUE CACHE BOOL
            "When this is present and set to true, it signals that we are building Qt from source.")
    endif()
endmacro()

macro(qt_internal_unset_extra_build_internals_vars)
    # Reset content of extra build internal vars for each inclusion of QtSetup.
    unset(QT_EXTRA_BUILD_INTERNALS_VARS)
endmacro()

macro(qt_internal_get_generator_is_multi_config)
    # Save the global property in a variable to make it available to feature conditions.
    get_property(QT_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
endmacro()

macro(qt_internal_setup_position_independent_code)
    ## Position independent code:
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # Does the linker support position independent code?
    include(CheckPIESupported)
    check_pie_supported()
endmacro()

macro(qt_internal_set_link_depends_no_shared)
    # Do not relink dependent libraries when no header has changed:
    set(CMAKE_LINK_DEPENDS_NO_SHARED ON)
endmacro()

macro(qt_internal_set_qt_source_tree_var)
    # Specify the QT_SOURCE_TREE only when building qtbase. Needed by some tests when the tests are
    # built as part of the project, and not standalone. For standalone tests, the value is set in
    # QtBuildInternalsExtra.cmake.
    if(PROJECT_NAME STREQUAL "QtBase")
        set(QT_SOURCE_TREE "${QtBase_SOURCE_DIR}" CACHE PATH
            "A path to the source tree of the previously configured QtBase project." FORCE)
    endif()
endmacro()

macro(qt_internal_include_qt_platform_android)
    ## Android platform settings
    if(ANDROID)
        include(QtPlatformAndroid)
    endif()
endmacro()

macro(qt_internal_include_qt_properties)
    include(QtProperties)
endmacro()

macro(qt_internal_set_compiler_optimization_flags)
    include(QtCompilerOptimization)
endmacro()

macro(qt_internal_set_compiler_warning_flags)
    include(QtCompilerFlags)
endmacro()

macro(qt_internal_set_skip_setup_deployment)
    if(NOT QT_BUILD_EXAMPLES)
        # Disable deployment setup to avoid warnings about missing patchelf with CMake < 3.21.
        set(QT_SKIP_SETUP_DEPLOYMENT ON)
    endif()
endmacro()

macro(qt_internal_reset_global_state)
    qt_internal_clear_qt_repo_known_modules()
    qt_internal_clear_qt_repo_known_plugin_types()
    qt_internal_set_qt_known_plugins("")

    set(QT_KNOWN_MODULES_WITH_TOOLS "" CACHE INTERNAL "Known Qt modules with tools" FORCE)
endmacro()

macro(qt_internal_set_qt_path_separator)
    # For adjusting variables when running tests, we need to know what
    # the correct variable is for separating entries in PATH-alike
    # variables.
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        set(QT_PATH_SEPARATOR "\\;")
    else()
        set(QT_PATH_SEPARATOR ":")
    endif()
endmacro()

macro(qt_internal_set_internals_extra_cmake_code)
    # This is used to hold extra cmake code that should be put into QtBuildInternalsExtra.cmake file
    # at the QtPostProcess stage.
    set(QT_BUILD_INTERNALS_EXTRA_CMAKE_CODE "")
endmacro()

macro(qt_internal_set_top_level_source_dir)
    # Save the value of the current first project source dir.
    # This will be /path/to/qtbase for qtbase both in a super-build and a non super-build.
    # This will be /path/to/qtbase/tests when building standalone tests.
    set(QT_TOP_LEVEL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endmacro()

macro(qt_internal_set_apple_archiver_flags)
    # Prevent warnings about object files without any symbols. This is a common
    # thing in Qt as we tend to build files unconditionally, and then use ifdefs
    # to compile out parts that are not relevant.
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
        foreach(lang ASM C CXX)
            # We have to tell 'ar' to not run ranlib by itself, by passing the 'S' option
            set(CMAKE_${lang}_ARCHIVE_CREATE "<CMAKE_AR> qcS <TARGET> <LINK_FLAGS> <OBJECTS>")
            set(CMAKE_${lang}_ARCHIVE_APPEND "<CMAKE_AR> qS <TARGET> <LINK_FLAGS> <OBJECTS>")
            set(CMAKE_${lang}_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols <TARGET>")
        endforeach()
    endif()
endmacro()

macro(qt_internal_set_apple_privacy_manifest target manifest_file)
    set_target_properties(${target} PROPERTIES _qt_privacy_manifest "${manifest_file}")
endmacro()

macro(qt_internal_set_debug_extend_target)
    option(QT_CMAKE_DEBUG_EXTEND_TARGET "Debug extend_target calls in Qt's build system" OFF)
endmacro()

# These upstream CMake modules will be automatically include()'d when doing
# find_package(Qt6 COMPONENTS BuildInternals).
function(qt_internal_get_qt_build_upstream_cmake_modules out_var)
    set(${out_var}
        CMakeFindBinUtils
        CMakePackageConfigHelpers
        CheckCXXSourceCompiles
        FeatureSummary
        PARENT_SCOPE
    )
endfunction()

# These helpers will be installed when building qtbase, and they will be automatically include()'d
# when doing find_package(Qt6 COMPONENTS BuildInternals).
# The helpers are expected to exist under the qtbase/cmake sub-directory and their file name
# extension should be '.cmake'.
function(qt_internal_get_qt_build_private_helpers out_var)
    set(${out_var}
        Qt3rdPartyLibraryHelpers
        QtAndroidHelpers
        QtAppHelpers
        QtAutoDetectHelpers
        QtAutogenHelpers
        QtBuildInformation
        QtBuildOptionsHelpers
        QtBuildPathsHelpers
        QtBuildRepoExamplesHelpers
        QtBuildRepoHelpers
        QtCMakeHelpers
        QtCMakeVersionHelpers
        QtDbusHelpers
        QtDeferredDependenciesHelpers
        QtDocsHelpers
        QtExecutableHelpers
        QtFindPackageHelpers
        QtFlagHandlingHelpers
        QtFrameworkHelpers
        QtGlobalStateHelpers
        QtHeadersClean
        QtInstallHelpers
        QtJavaHelpers
        QtLalrHelpers
        QtMkspecHelpers
        QtModuleHelpers
        QtNoLinkTargetHelpers
        QtPkgConfigHelpers
        QtPlatformTargetHelpers
        QtPluginHelpers
        QtPostProcessHelpers
        QtPrecompiledHeadersHelpers
        QtPriHelpers
        QtPrlHelpers
        QtProperties
        QtQmakeHelpers
        QtResourceHelpers
        QtRpathHelpers
        QtSanitizerHelpers
        QtSbomHelpers
        QtScopeFinalizerHelpers
        QtSeparateDebugInfo
        QtSimdHelpers
        QtSingleRepoTargetSetBuildHelpers
        QtSyncQtHelpers
        QtTargetHelpers
        QtTestHelpers
        QtToolHelpers
        QtToolchainHelpers
        QtUnityBuildHelpers
        QtWasmHelpers
        QtWindowsHelpers
        QtWrapperScriptHelpers
        PARENT_SCOPE
    )
endfunction()

# These files will be installed when building qtbase, but will NOT be automatically include()d
# when doing find_package(Qt6 COMPONENTS BuildInternals).
# The files are expected to exist under the qtbase/cmake sub-directory.
function(qt_internal_get_qt_build_private_files_to_install out_var)
    set(${out_var}
        ModuleDescription.json.in
        PkgConfigLibrary.pc.in
        Qt3rdPartyLibraryConfig.cmake.in
        QtTransitiveExtras.cmake.in
        QtBaseTopLevelHelpers.cmake
        QtBuild.cmake
        QtBuildHelpers.cmake
        QtBuildStaticDocToolsScript.cmake
        QtCMakePackageVersionFile.cmake.in
        QtCompilerFlags.cmake
        QtCompilerOptimization.cmake
        QtConfigDependencies.cmake.in
        QtConfigureTimeExecutableCMakeLists.txt.in
        QtFileConfigure.txt.in
        QtFindWrapConfigExtra.cmake.in
        QtFindWrapHelper.cmake
        QtFinishPkgConfigFile.cmake
        QtFinishPrlFile.cmake
        QtGenerateExtPri.cmake
        QtGenerateLibHelpers.cmake
        QtGenerateLibPri.cmake
        QtGenerateVersionScript.cmake
        QtModuleConfig.cmake.in
        QtModuleConfigPrivate.cmake.in
        QtModuleDependencies.cmake.in
        QtModuleHeadersCheck.cmake
        QtModuleToolsConfig.cmake.in
        QtModuleToolsDependencies.cmake.in
        QtModuleToolsVersionlessTargets.cmake.in
        QtPlatformAndroid.cmake
        QtPlatformSupport.cmake
        QtPluginConfig.cmake.in
        QtPluginDependencies.cmake.in
        QtPlugins.cmake.in
        QtPostProcess.cmake
        QtProcessConfigureArgs.cmake
        QtSeparateDebugInfo.Info.plist.in
        QtSetup.cmake
        QtStandaloneTestsConfig.cmake.in
        QtVersionlessAliasTargets.cmake.in
        QtVersionlessTargets.cmake.in
        QtWriteArgsFile.cmake
        modulecppexports.h.in
        qbatchedtestrunner.in.cpp
        qt-internal-config.redo.in
        qt-internal-config.redo.bat.in
        PARENT_SCOPE
    )
endfunction()

# These helpers will be installed when building qtbase, and they will be automatically include()'d
# when doing find_package(Qt6 COMPONENTS BuildInternals).
# The helpers are expected to exist under the qtbase/cmake sub-directory and their file name
# extension should be '.cmake'.
# In addition, they are meant to be included when doing find_package(Qt6) as well.
function(qt_internal_get_qt_build_public_helpers out_var)
    set(${out_var}
        QtFeature
        QtFeatureCommon
        QtPublicAndroidHelpers
        QtPublicAppleHelpers
        QtPublicCMakeHelpers
        QtPublicCMakeVersionHelpers
        QtPublicDependencyHelpers
        QtPublicExternalProjectHelpers
        QtPublicFinalizerHelpers
        QtPublicFindPackageHelpers
        QtPublicGitHelpers
        QtPublicPluginHelpers
        QtPublicPluginHelpers_v2
        QtPublicSbomAttributionHelpers
        QtPublicSbomBuildToolHelpers
        QtPublicSbomCommonGenerationHelpers
        QtPublicSbomCpeHelpers
        QtPublicSbomCycloneDXHelpers
        QtPublicSbomDocumentNamespaceHelpers
        QtPublicSbomDepHelpers
        QtPublicSbomExternalReferenceHelpers
        QtPublicSbomFileHelpers
        QtPublicSbomGenerationHelpers
        QtPublicSbomGenerationCycloneDXHelpers
        QtPublicSbomHelpers
        QtPublicSbomLicenseHelpers
        QtPublicSbomOpsHelpers
        QtPublicSbomPurlHelpers
        QtPublicSbomPythonHelpers
        QtPublicSbomQtEntityHelpers
        QtPublicSbomRelationshipHelpers
        QtPublicSbomSystemDepHelpers
        QtPublicTargetHelpers
        QtPublicTestHelpers
        QtPublicToolHelpers
        QtPublicWalkLibsHelpers
        QtPublicWindowsHelpers
        PARENT_SCOPE
    )
endfunction()

# These files will be installed when building qtbase, but will NOT be automatically include()d
# when doing find_package(Qt6) nor find_package(Qt6 COMPONENTS BuildInternals).
# The files are expected to exist under the qtbase/cmake sub-directory.
function(qt_internal_get_qt_build_public_files_to_install out_var)
    set(${out_var}
        QtCopyFileIfDifferent.cmake
        QtInitProject.cmake
        QtPublicCMakeEarlyPolicyHelpers.cmake

        # Public CMake files that are installed next Qt6Config.cmake, but are NOT included by it.
        # Instead they are included by the generated CMake toolchain file.
        QtPublicWasmToolchainHelpers.cmake

        PARENT_SCOPE
    )
endfunction()

# Includes all Qt CMake helper files that define functions and macros.
macro(qt_internal_include_all_helpers)
    # Upstream cmake modules.
    qt_internal_get_qt_build_upstream_cmake_modules(__qt_upstream_helpers)
    foreach(__qt_file_name IN LISTS __qt_upstream_helpers)
        include("${__qt_file_name}")
    endforeach()

    # Internal helpers available only while building Qt itself.
    qt_internal_get_qt_build_private_helpers(__qt_private_helpers)
    foreach(__qt_file_name IN LISTS __qt_private_helpers)
        include("${__qt_file_name}")
    endforeach()

    # Helpers that are available in public projects and while building Qt itself.
    qt_internal_get_qt_build_public_helpers(__qt_public_helpers)
    foreach(__qt_file_name IN LISTS __qt_public_helpers)
        include("${__qt_file_name}")
    endforeach()
endmacro()

function(qt_internal_check_host_path_set_for_cross_compiling)
    if(CMAKE_CROSSCOMPILING)
        if(NOT IS_DIRECTORY "${QT_HOST_PATH}")
            message(FATAL_ERROR "You need to set QT_HOST_PATH to cross compile Qt.")
        endif()
    endif()
endfunction()

macro(qt_internal_setup_find_host_info_package)
    _qt_internal_determine_if_host_info_package_needed(__qt_build_requires_host_info_package)
    _qt_internal_find_host_info_package("${__qt_build_requires_host_info_package}"
        ${INSTALL_CMAKE_NAMESPACE})
endmacro()

macro(qt_internal_setup_poor_mans_scope_finalizer)
    # This sets up the poor man's scope finalizer mechanism.
    # For newer CMake versions, we use cmake_language(DEFER CALL) instead.
    if(CMAKE_VERSION VERSION_LESS "3.19.0")
        variable_watch(CMAKE_CURRENT_LIST_DIR qt_watch_current_list_dir)
    endif()
endmacro()

macro(qt_internal_set_qt_namespace)
    set(QT_NAMESPACE "" CACHE STRING "Qt Namespace")
endmacro()

macro(qt_internal_set_qt_coord_type)
    if(PROJECT_NAME STREQUAL "QtBase")
        set(QT_COORD_TYPE double CACHE STRING "Type of qreal")
    endif()
endmacro()

function(qt_internal_check_macos_host_version)
    # macOS versions 10.14 and less don't have the implementation of std::filesystem API.
    if(CMAKE_HOST_APPLE AND CMAKE_HOST_SYSTEM_VERSION VERSION_LESS "19.0.0")
        message(FATAL_ERROR "macOS versions less than 10.15 are not supported for building Qt.")
    endif()
endfunction()

function(qt_internal_setup_tool_path_command)
    if(NOT CMAKE_HOST_WIN32)
        return()
    endif()
    set(bindir "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    file(TO_NATIVE_PATH "${bindir}" bindir)
    list(APPEND command COMMAND)
    list(APPEND command set PATH=${bindir}$<SEMICOLON>%PATH%)
    set(QT_TOOL_PATH_SETUP_COMMAND "${command}" CACHE INTERNAL
        "internal command prefix for tool invocations" FORCE)
    # QT_TOOL_PATH_SETUP_COMMAND is deprecated. Please use _qt_internal_get_wrap_tool_script_path
    # instead.
endfunction()

macro(qt_internal_setup_android_platform_specifics)
    if(ANDROID)
        qt_internal_setup_android_target_properties()
    endif()
endmacro()

macro(qt_internal_setup_build_and_global_variables)
    qt_internal_validate_cmake_generator()
    qt_internal_set_qt_building_qt()
    qt_internal_set_cmake_build_type()
    qt_internal_set_message_log_level(CMAKE_MESSAGE_LOG_LEVEL)
    qt_internal_unset_extra_build_internals_vars()
    qt_internal_get_generator_is_multi_config()

    # Depends on qt_internal_set_cmake_build_type
    qt_internal_setup_cmake_config_postfix()

    qt_internal_setup_position_independent_code()
    qt_internal_set_link_depends_no_shared()
    qt_internal_setup_default_install_prefix()
    qt_internal_set_qt_source_tree_var()
    qt_internal_set_export_compile_commands()
    qt_internal_set_configure_from_ide()

    # Depends on qt_internal_set_configure_from_ide
    qt_internal_set_sync_headers_at_configure_time()

    qt_internal_setup_build_benchmarks()

    # Depends on qt_internal_setup_build_benchmarks
    qt_internal_setup_build_tests()

    qt_internal_setup_build_tools()

    qt_internal_setup_sbom()

    # Depends on qt_internal_setup_default_install_prefix
    qt_internal_setup_build_examples()

    qt_internal_set_qt_host_path()
    qt_internal_setup_find_host_info_package()

    qt_internal_setup_build_docs()
    qt_internal_setup_build_java_docs_on_host()

    qt_internal_include_qt_platform_android()

    qt_internal_include_qt_properties()

    # Depends on qt_internal_setup_default_install_prefix
    qt_internal_setup_paths_and_prefixes()

    qt_internal_reset_global_state()

    # Depends on qt_internal_setup_paths_and_prefixes
    qt_internal_set_mkspecs_dir()
    qt_internal_setup_platform_definitions_and_mkspec()

    qt_internal_check_macos_host_version()
    _qt_internal_check_apple_sdk_and_xcode_versions()
    qt_internal_check_msvc_versions()
    qt_internal_check_host_path_set_for_cross_compiling()
    qt_internal_setup_android_platform_specifics()
    qt_internal_setup_tool_path_command()
    qt_internal_setup_default_target_function_options()
    qt_internal_set_default_rpath_settings()
    qt_internal_set_qt_namespace()
    qt_internal_set_qt_coord_type()
    qt_internal_set_qt_path_separator()
    qt_internal_set_internals_extra_cmake_code()
    qt_internal_set_top_level_source_dir()
    qt_internal_set_apple_archiver_flags()
    qt_internal_set_debug_extend_target()
    qt_internal_setup_poor_mans_scope_finalizer()

    qt_internal_set_compiler_optimization_flags()
    qt_internal_set_compiler_warning_flags()

    qt_set_language_standards()
    qt_internal_set_use_ccache()
    qt_internal_set_allow_symlink_in_paths()
    qt_internal_set_skip_setup_deployment()
    qt_internal_set_qt_allow_download()

    qt_internal_detect_dirty_features()
endmacro()
