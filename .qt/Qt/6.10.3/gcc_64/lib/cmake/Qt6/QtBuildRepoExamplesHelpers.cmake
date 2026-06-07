# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

macro(qt_examples_build_begin)
    set(options EXTERNAL_BUILD)
    set(singleOpts "")
    set(multiOpts DEPENDS)

    cmake_parse_arguments(arg "${options}" "${singleOpts}" "${multiOpts}" ${ARGN})

    # Examples are not unity-ready.
    set(CMAKE_UNITY_BUILD OFF)

    qt_internal_sbom_disable_sbom_for_examples_subdir()

    # Skip running deployment steps when the developer asked to deploy a minimal subset of examples.
    # Each example can then decide whether it wants to be deployed as part of the minimal subset
    # by unsetting the QT_INTERNAL_SKIP_DEPLOYMENT variable before its qt_internal_add_example call.
    # This will be used by our CI.
    if(NOT DEFINED QT_INTERNAL_SKIP_DEPLOYMENT AND QT_DEPLOY_MINIMAL_EXAMPLES)
        set(QT_INTERNAL_SKIP_DEPLOYMENT TRUE)
    endif()

    # Use by qt_internal_add_example.
    set(QT_EXAMPLE_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    if(QT_BUILD_STANDALONE_EXAMPLES)
        # Find all qt packages, so that the various if(QT_FEATURE_foo) add_subdirectory()
        # conditions have correct values, regardless whether we will use ExternalProjects or not.
        qt_internal_find_standalone_parts_config_files()
    endif()

    string(TOLOWER ${PROJECT_NAME} project_name_lower)

    if(arg_EXTERNAL_BUILD AND QT_BUILD_EXAMPLES_AS_EXTERNAL)
        # Examples will be built using ExternalProject.
        # We depend on all plugins built as part of the current repo as well as current repo's
        # dependencies plugins, to prevent opportunities for
        # weird errors associated with loading out-of-date plugins from
        # unrelated Qt modules.
        # We also depend on all targets from this repo's src and tools subdirectories
        # to ensure that we've built anything that a find_package() call within
        # an example might use. Projects can add further dependencies if needed,
        # but that should rarely be necessary.
        set(QT_EXAMPLE_DEPENDENCIES ${qt_repo_plugins_recursive} ${arg_DEPENDS})

        if(TARGET ${qt_repo_targets_name}_src)
            list(APPEND QT_EXAMPLE_DEPENDENCIES ${qt_repo_targets_name}_src_for_examples)
        endif()

        if(TARGET ${qt_repo_targets_name}_tools)
            list(APPEND QT_EXAMPLE_DEPENDENCIES ${qt_repo_targets_name}_tools)
        endif()

        set(QT_IS_EXTERNAL_EXAMPLES_BUILD TRUE)

        if(NOT TARGET examples)
            if(QT_BUILD_EXAMPLES_BY_DEFAULT)
                add_custom_target(examples ALL)
            else()
                add_custom_target(examples)
            endif()
        endif()
        if(NOT TARGET examples_${project_name_lower})
            add_custom_target(examples_${project_name_lower})
            add_dependencies(examples examples_${project_name_lower})
        endif()

        include(ExternalProject)
    else()
        # This repo has not yet been updated to build examples in a separate
        # build from this main build, or we can't use that arrangement yet.
        # Build them directly as part of the main build instead for backward
        # compatibility.
        if(NOT BUILD_SHARED_LIBS)
            # Ordinarily, it would be an error to call return() from within a
            # macro(), but in this case we specifically want to return from the
            # caller's scope if we are doing a static build and the project
            # isn't building examples in a separate build from the main build.
            # Configuring static builds requires tools that are not available
            # until build time.
            return()
        endif()

        if(NOT QT_BUILD_EXAMPLES_BY_DEFAULT)
            set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL TRUE)
        endif()

        # Add active linker flags to all targets created below this subdirectory.
        qt_internal_get_active_linker_flags(active_linker_flags)
        if(active_linker_flags)
            add_link_options(${active_linker_flags})
        endif()

        # Marker for warnings_as_errors.
        set(QT_INTERNAL_IS_EXAMPLE_IN_TREE_BUILD ON)
    endif()

    # TODO: Change this to TRUE when all examples in all repos are ported to use
    # qt_internal_add_example.
    # We shouldn't need to call qt_internal_set_up_build_dir_package_paths when
    # QT_IS_EXTERNAL_EXAMPLES_BUILD is TRUE.
    # Due to not all examples being ported, if we don't
    # call qt_internal_set_up_build_dir_package_paths -> set(QT_NO_CREATE_TARGETS TRUE) we'll get
    # CMake configuration errors saying we redefine Qt targets because we both build them and find
    # them as part of find_package.
    set(__qt_all_examples_ported_to_external_projects FALSE)

    # Examples that are built as part of the Qt build need to use the CMake config files from the
    # build dir, because they are not installed yet in a prefix build.
    # Prepending to CMAKE_PREFIX_PATH helps find the initial Qt6Config.cmake.
    # Prepending to QT_BUILD_CMAKE_PREFIX_PATH helps find components of Qt6, because those
    # find_package calls use NO_DEFAULT_PATH, and thus CMAKE_PREFIX_PATH is ignored.
    # Prepending to CMAKE_FIND_ROOT_PATH ensures the components are found while cross-compiling
    # without setting CMAKE_FIND_ROOT_PATH_MODE_PACKAGE to BOTH.
    if(NOT QT_IS_EXTERNAL_EXAMPLES_BUILD OR NOT __qt_all_examples_ported_to_external_projects)
        qt_internal_set_up_build_dir_package_paths()
        list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_BUILD_DIR}")
        list(PREPEND QT_BUILD_CMAKE_PREFIX_PATH "${QT_BUILD_DIR}/${INSTALL_LIBDIR}/cmake")
    endif()

    # Because CMAKE_INSTALL_RPATH is empty by default in the repo project, examples need to have
    # it set here, so they can run when installed.
    # This means that installed examples are not relocatable at the moment. We would need to
    # annotate where each example is installed to, to be able to derive a relative rpath, and it
    # seems there's no way to query such information from CMake itself.
    set(CMAKE_INSTALL_RPATH "${_default_install_rpath}")

    install(CODE "
# Backup CMAKE_INSTALL_PREFIX because we're going to change it in each example subdirectory
# and restore it after all examples are processed so that QtFooToolsAdditionalTargetInfo.cmake
# files are installed into the original install prefix.
set(_qt_internal_examples_cmake_install_prefix_backup \"\${CMAKE_INSTALL_PREFIX}\")
")

    # Backup the DESTDIR and unset it, so that example installation is not affected by DESTDIR.
    # This is activated by our CI when QT_INTERNAL_EXAMPLES_INSTALL_PREFIX is set.
    if(QT_INTERNAL_EXAMPLES_INSTALL_PREFIX)
        set(_qt_examples_should_unset_destdir TRUE)
    else()
        set(_qt_examples_should_unset_destdir FALSE)
    endif()

    set_property(GLOBAL PROPERTY
        _qt_examples_should_unset_destdir_${project_name_lower}
        "${_qt_examples_should_unset_destdir}")
    if(_qt_examples_should_unset_destdir)
        install(CODE "
    # Temporarily unset DESTDIR while examples are being installed.
    set(_qt_internal_examples_destdir_backup \"\$ENV{DESTDIR}\")
    unset(ENV{DESTDIR})
")
    endif()

    unset(_qt_examples_should_unset_destdir)
    unset(project_name_lower)
endmacro()

macro(qt_examples_build_end)
    # We use AUTOMOC/UIC/RCC in the examples. When the examples are part of the
    # main build rather than being built in their own separate project, make
    # sure we do not fail on a fresh Qt build (e.g. the moc binary won't exist
    # yet because it is created at build time).

    _qt_internal_collect_buildsystem_targets(targets
            "${CMAKE_CURRENT_SOURCE_DIR}" EXCLUDE UTILITY ALIAS)

    foreach(target ${targets})
        # Skip re-enabling AUTMOC for object libraries created by _qt_internal_add_rcc_pass2,
        # to avoid build errors.
        get_target_property(is_rcc_pass2_obj_lib "${target}" _qt_internal_is_rcc_pass2_obj_lib)
        if(is_rcc_pass2_obj_lib)
            continue()
        endif()

        qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS "moc" "rcc")
        if(TARGET Qt::Widgets)
            qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS "uic")
        endif()
        set_target_properties(${target} PROPERTIES UNITY_BUILD OFF)
    endforeach()

    install(CODE "
# Restore backed up CMAKE_INSTALL_PREFIX.
set(CMAKE_INSTALL_PREFIX \"\${_qt_internal_examples_cmake_install_prefix_backup}\")
")

    set(CMAKE_UNITY_BUILD ${QT_UNITY_BUILD})

    string(TOLOWER ${PROJECT_NAME} project_name_lower)
    get_property(_qt_examples_should_unset_destdir
        GLOBAL PROPERTY _qt_examples_should_unset_destdir_${project_name_lower})

    if(_qt_examples_should_unset_destdir)
        install(CODE "
    # Restore the DESTDIR env var after examples have been installed.
    set(ENV{DESTDIR} \"\${_qt_internal_examples_destdir_backup}\")
    unset(_qt_internal_examples_destdir_backup)
")
    endif()

    unset(_qt_examples_should_unset_destdir)
    unset(project_name_lower)
endmacro()

# Allows building an example either as an ExternalProject or in-tree with the Qt build.
# Also allows installing the example sources.
function(qt_internal_add_example subdir)
    # Don't show warnings for examples that were added via qt_internal_add_example.
    # Those that are added via add_subdirectory will see the warning, due to the parent scope
    # having the variable set to TRUE.
    if(QT_FEATURE_developer_build AND NOT QT_NO_WARN_ABOUT_EXAMPLE_ADD_SUBDIRECTORY_WARNING)
        set(QT_WARN_ABOUT_EXAMPLE_ADD_SUBDIRECTORY FALSE)
    endif()

    # Pre-compute unique example name based on the subdir, in case of target name clashes.
    qt_internal_get_example_unique_name(unique_example_name "${subdir}")

    # QT_INTERNAL_NO_CONFIGURE_EXAMPLES is not meant to be used by Qt builders, it's here for faster
    # testing of the source installation code path for build system engineers.
    if(NOT QT_INTERNAL_NO_CONFIGURE_EXAMPLES)
        if(NOT QT_IS_EXTERNAL_EXAMPLES_BUILD)
            qt_internal_add_example_in_tree("${subdir}")
        else()
            qt_internal_add_example_external_project("${subdir}"
                NAME "${unique_example_name}")
        endif()
    endif()

    if(QT_INSTALL_EXAMPLES_SOURCES)
        string(TOLOWER ${PROJECT_NAME} project_name_lower)

        qt_internal_install_example_sources("${subdir}"
            NAME "${unique_example_name}"
            REPO_NAME "${project_name_lower}")
    endif()
endfunction()

# Gets the install prefix where an example should be installed.
# Used for computing the final installation path.
function(qt_internal_get_example_install_prefix out_var)
    # Allow customizing the installation path of the examples. Will be used in CI.
    if(QT_INTERNAL_EXAMPLES_INSTALL_PREFIX)
        set(qt_example_install_prefix "${QT_INTERNAL_EXAMPLES_INSTALL_PREFIX}")
    elseif(QT_BUILD_STANDALONE_EXAMPLES AND NOT QT_NO_FAKE_STANDALONE_EXAMPLE_INSTALL_PREFIX)
        # TODO: We might need to reset and pipe through an empty CMAKE_STAGING_PREFIX if we ever
        # try to run standalone examples in the CI when cross-compiling, similar how it's done in
        # qt_internal_set_up_fake_standalone_parts_install_prefix.
        qt_internal_get_fake_standalone_install_prefix(qt_example_install_prefix)
    else()
        set(qt_example_install_prefix "${CMAKE_INSTALL_PREFIX}/${INSTALL_EXAMPLESDIR}")
    endif()
    file(TO_CMAKE_PATH "${qt_example_install_prefix}" qt_example_install_prefix)
    set(${out_var} "${qt_example_install_prefix}" PARENT_SCOPE)
endfunction()

# Gets the install prefix where an example's sources should be installed.
# Used for computing the final installation path.
function(qt_internal_get_examples_sources_install_prefix out_var)
    # Allow customizing the installation path of the examples source specifically.
    if(QT_INTERNAL_EXAMPLES_SOURCES_INSTALL_PREFIX)
        set(qt_example_install_prefix "${QT_INTERNAL_EXAMPLES_SOURCES_INSTALL_PREFIX}")
    else()
        qt_internal_get_example_install_prefix(qt_example_install_prefix)
    endif()
    file(TO_CMAKE_PATH "${qt_example_install_prefix}" qt_example_install_prefix)
    set(${out_var} "${qt_example_install_prefix}" PARENT_SCOPE)
endfunction()

# Gets the relative path of an example, relative to the current repo's examples source dir.
# QT_EXAMPLE_BASE_DIR is meant to be already set in a parent scope.
function(qt_internal_get_example_rel_path out_var subdir)
    file(RELATIVE_PATH example_rel_path
         "${QT_EXAMPLE_BASE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
    set(${out_var} "${example_rel_path}" PARENT_SCOPE)
endfunction()

# Gets the install path where an example should be installed.
function(qt_internal_get_example_install_path out_var subdir)
    qt_internal_get_example_install_prefix(qt_example_install_prefix)
    qt_internal_get_example_rel_path(example_rel_path "${subdir}")
    set(example_install_path "${qt_example_install_prefix}/${example_rel_path}")

    set(${out_var} "${example_install_path}" PARENT_SCOPE)
endfunction()

# Gets the install path where an example's sources should be installed.
function(qt_internal_get_examples_sources_install_path out_var subdir)
    qt_internal_get_examples_sources_install_prefix(qt_example_install_prefix)
    qt_internal_get_example_rel_path(example_rel_path "${subdir}")
    set(example_install_path "${qt_example_install_prefix}/${example_rel_path}")

    set(${out_var} "${example_install_path}" PARENT_SCOPE)
endfunction()

# Get the unique name of an example project based on its subdir or explicitly given name.
# Makes the name unique by appending a short sha1 hash of the relative path of the example
# if a target of the same name already exist.
function(qt_internal_get_example_unique_name out_var subdir)
    qt_internal_get_example_rel_path(example_rel_path "${subdir}")

    set(name "${subdir}")

    # qtdeclarative has calls like qt_internal_add_example(imagine/automotive)
    # so passing a nested subdirectory. Custom targets (and thus ExternalProjects) can't contain
    # slashes, so extract the last part of the path to be used as a name.
    if(name MATCHES "/")
        string(REPLACE "/" ";" exploded_path "${name}")
        list(POP_BACK exploded_path last_dir)
        if(NOT last_dir)
            message(FATAL_ERROR "Example subdirectory must have a name.")
        else()
            set(name "${last_dir}")
        endif()
    endif()

    # Likely a clash with an example subdir ExternalProject custom target of the same name in a
    # top-level build.
    if(TARGET "${name}")
        string(SHA1 rel_path_hash "${example_rel_path}")
        string(SUBSTRING "${rel_path_hash}" 0 4 short_hash)
        set(name "${name}-${short_hash}")
    endif()

    set(${out_var} "${name}" PARENT_SCOPE)
endfunction()

# Use old non-ExternalProject approach, aka build in-tree with the Qt build.
function(qt_internal_add_example_in_tree subdir)
    # Unset the default CMAKE_INSTALL_PREFIX that's generated in
    #   ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
    # so we can override it with a different value in
    #   ${CMAKE_CURRENT_BINARY_DIR}/${subdir}/cmake_install.cmake
    #
    install(CODE "
# Unset the CMAKE_INSTALL_PREFIX in the current cmake_install.cmake file so that it can be
# overridden in the included add_subdirectory-specific cmake_install.cmake files instead.
# Also unset the deployment prefix, so it can be recomputed for each example subdirectory.
unset(CMAKE_INSTALL_PREFIX)
unset(QT_DEPLOY_PREFIX)
")

    # Override the install prefix in the subdir cmake_install.cmake, so that
    # relative install(TARGETS DESTINATION) calls in example projects install where we tell them to.
    qt_internal_get_example_install_path(example_install_path "${subdir}")
    set(CMAKE_INSTALL_PREFIX "${example_install_path}")

    # Make sure unclean example projects have their INSTALL_EXAMPLEDIR set to "."
    # Won't have any effect on example projects that don't use INSTALL_EXAMPLEDIR.
    # This plus the install prefix above takes care of installing examples where we want them to
    # be installed, while allowing us to remove INSTALL_EXAMPLEDIR code in each example
    # incrementally.
    # TODO: Remove once all repositories use qt_internal_add_example instead of add_subdirectory.
    set(QT_INTERNAL_SET_EXAMPLE_INSTALL_DIR_TO_DOT ON)

    add_subdirectory(${subdir})
endfunction()

function(qt_internal_add_example_external_project subdir)
    set(options "")
    set(singleOpts NAME)
    set(multiOpts "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${singleOpts}" "${multiOpts}")

    _qt_internal_get_build_vars_for_external_projects(
        CMAKE_DIR_VAR qt_cmake_dir
        PREFIXES_VAR qt_prefixes
        ADDITIONAL_PACKAGES_PREFIXES_VAR qt_additional_packages_prefixes
    )

    list(APPEND QT_ADDITIONAL_PACKAGES_PREFIX_PATH "${qt_additional_packages_prefixes}")

    set(vars_to_pass_if_defined)
    set(var_defs)
    if(QT_HOST_PATH OR CMAKE_CROSSCOMPILING)
        list(APPEND var_defs
            -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${qt_cmake_dir}/qt.toolchain.cmake
        )
    else()
        list(PREPEND CMAKE_PREFIX_PATH ${qt_prefixes})

        # Setting CMAKE_SYSTEM_NAME affects CMAKE_CROSSCOMPILING, even if it is
        # set to the same as the host, so it should only be set if it is different.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/21744
        if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND
           NOT CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
            list(APPEND vars_to_pass_if_defined CMAKE_SYSTEM_NAME:STRING)
        endif()
    endif()

    # We we need to augment the CMAKE_MODULE_PATH with the current repo cmake build dir, to find
    # files like FindWrapBundledFooConfigExtra.cmake.
    set(module_paths "${qt_prefixes}")
    list(TRANSFORM module_paths APPEND "/${INSTALL_LIBDIR}/cmake/${QT_CMAKE_EXPORT_NAMESPACE}")
    list(APPEND CMAKE_MODULE_PATH ${module_paths})

    # Pass additional paths where qml plugin config files should be included by Qt6QmlPlugins.cmake.
    # This is needed in prefix builds, where the cmake files are not installed yet.
    set(glob_prefixes "${qt_prefixes}")
    list(TRANSFORM glob_prefixes APPEND "/${INSTALL_LIBDIR}/cmake/${QT_CMAKE_EXPORT_NAMESPACE}Qml")

    set(qml_plugin_cmake_config_file_glob_prefixes "")
    foreach(glob_prefix IN LISTS glob_prefixes)
        if(EXISTS "${glob_prefix}")
            list(APPEND qml_plugin_cmake_config_file_glob_prefixes "${glob_prefix}")
        endif()
    endforeach()

    if(qml_plugin_cmake_config_file_glob_prefixes)
        set(QT_ADDITIONAL_QML_PLUGIN_GLOB_PREFIXES ${qml_plugin_cmake_config_file_glob_prefixes})
    endif()

    # In multi-config mode by default we exclude building tools for configs other than the main one.
    # Trying to build an example in a non-default config using the non-installed
    # QtFooConfig.cmake files would error out saying moc is not found.
    # Make sure to build examples only with the main config.
    # When users build an example against an installed Qt they won't have this problem because
    # the generated non-main QtFooTargets-$<CONFIG>.cmake file is empty and doesn't advertise
    # a tool that is not there.
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(CMAKE_CONFIGURATION_TYPES "${QT_MULTI_CONFIG_FIRST_CONFIG}")
    endif()

    # We need to pass the modified CXX flags of the parent project so that using sccache works
    # properly and doesn't error out due to concurrent access to the pdb files.
    # See qt_internal_set_up_config_optimizations_like_in_qmake, "/Zi" "/Z7".
    if(MSVC AND QT_FEATURE_msvc_obj_debug_info)
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
        set(configs RELWITHDEBINFO DEBUG)
        foreach(lang ${enabled_languages})
            foreach(config ${configs})
                set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
                list(APPEND vars_to_pass_if_defined "${flag_var_name}:STRING")
            endforeach()
        endforeach()
    endif()

    # When cross-compiling for a qemu target in our CI, we source an environment script
    # that sets environment variables like CC and CXX. These are parsed by CMake on initial
    # configuration to populate the cache vars CMAKE_${lang}_COMPILER.
    # If the environment variable specified not only the compiler path, but also a list of flags
    # to pass to the compiler, CMake parses those out into a separate CMAKE_${lang}_COMPILER_ARG1
    # cache variable. In such a case, we want to ensure that the external project also sees those
    # flags.
    # Unfortunately we can't do that by simply forwarding CMAKE_${lang}_COMPILER_ARG1 to the EP
    # because it breaks the compiler identification try_compile call, it simply doesn't consider
    # the cache var. From what I could gather, it's a limitation of try_compile and the list
    # of variables it considers for forwarding.
    # To fix this case, we ensure not to pass either cache variable, and let the external project
    # and its compiler identification try_compile project pick up the compiler and the flags
    # from the environment variables instead.
    foreach(lang_as_env_var CC CXX OBJC OBJCXX)
        if(lang_as_env_var STREQUAL "CC")
            set(lang_as_cache_var "C")
        else()
            set(lang_as_cache_var "${lang_as_env_var}")
        endif()
        set(lang_env_value "$ENV{${lang_as_env_var}}")
        if(lang_env_value
                AND CMAKE_${lang_as_cache_var}_COMPILER
                AND CMAKE_${lang_as_cache_var}_COMPILER_ARG1)
            # The compiler environment variable is set and specifies a list of extra flags, don't
            # forward the compiler cache vars and rely on the environment variable to be picked up
            # instead.
        else()
            list(APPEND vars_to_pass_if_defined "CMAKE_${lang_as_cache_var}_COMPILER:STRING")
        endif()
    endforeach()
    unset(lang_as_env_var)
    unset(lang_as_cache_var)
    unset(lang_env_value)

    list(APPEND vars_to_pass_if_defined
        CMAKE_BUILD_TYPE:STRING
        CMAKE_CONFIGURATION_TYPES:STRING
        CMAKE_PREFIX_PATH:STRING
        QT_BUILD_CMAKE_PREFIX_PATH:STRING
        QT_ADDITIONAL_PACKAGES_PREFIX_PATH:STRING
        QT_ADDITIONAL_QML_PLUGIN_GLOB_PREFIXES:STRING
        QT_INTERNAL_SKIP_DEPLOYMENT:BOOL
        QT_REPO_EXAMPLES_WARNINGS_CLEAN:BOOL
        WARNINGS_ARE_ERRORS:BOOL
        CMAKE_FIND_ROOT_PATH:STRING
        CMAKE_MODULE_PATH:STRING
        BUILD_SHARED_LIBS:BOOL
        CMAKE_OSX_ARCHITECTURES:STRING
        CMAKE_OSX_DEPLOYMENT_TARGET:STRING
        CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED:BOOL
        CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL
        CMAKE_C_COMPILER_LAUNCHER:STRING
        CMAKE_CXX_COMPILER_LAUNCHER:STRING
        CMAKE_OBJC_COMPILER_LAUNCHER:STRING
        CMAKE_OBJCXX_COMPILER_LAUNCHER:STRING
    )

    # QT_EXAMPLE_CMAKE_VARS_TO_PASS can be set by specific repos to pass any additional required
    # CMake cache variables.
    # One use case is passing locations of 3rd party package locations like Protobuf via _ROOT
    # variables.
    set(extra_vars_var_name "")
    if(QT_EXAMPLE_CMAKE_VARS_TO_PASS)
        set(extra_vars_var_name "QT_EXAMPLE_CMAKE_VARS_TO_PASS")
    endif()
    foreach(var_with_type IN LISTS vars_to_pass_if_defined ${extra_vars_var_name})
        string(REPLACE ":" ";" key_as_list "${var_with_type}")
        list(GET key_as_list 0 var)
        if(NOT DEFINED ${var})
            continue()
        endif()

        # Preserve lists
        string(REPLACE ";" "$<SEMICOLON>" varForGenex "${${var}}")

        list(APPEND var_defs -D${var_with_type}=${varForGenex})
    endforeach()

    if(QT_INTERNAL_VERBOSE_EXAMPLES)
        list(APPEND var_defs -DCMAKE_MESSAGE_LOG_LEVEL:STRING=DEBUG)
        list(APPEND var_defs -DCMAKE_AUTOGEN_VERBOSE:BOOL=TRUE)
    endif()

    # Pass active linker flags.
    qt_internal_get_active_linker_flags(active_linker_flags)
    if(active_linker_flags)
        foreach(item_type EXE MODULE SHARED)
            list(APPEND var_defs
                "-DCMAKE_${item_type}_LINKER_FLAGS_INIT:STRING=${active_linker_flags}")
        endforeach()
    endif()

    set(deps "")
    list(REMOVE_DUPLICATES QT_EXAMPLE_DEPENDENCIES)
    foreach(dep IN LISTS QT_EXAMPLE_DEPENDENCIES)
        if(TARGET ${dep})
            list(APPEND deps ${dep})
        endif()
    endforeach()

    set(independent_args)
    cmake_policy(PUSH)
    if(POLICY CMP0114)
        set(independent_args INDEPENDENT TRUE)
        cmake_policy(SET CMP0114 NEW)
    endif()

    # The USES_TERMINAL_BUILD setting forces the build step to the console pool
    # when using Ninja. This has two benefits:
    #
    #   - You see build output as it is generated instead of at the end of the
    #     build step.
    #   - Only one task can use the console pool at a time, so it effectively
    #     serializes all example build steps, thereby preventing CPU
    #     over-commitment.
    #
    # If the loss of interactivity is not so important, one can allow CPU
    # over-commitment for Ninja builds. This may result in better throughput,
    # but is not allowed by default because it can make a machine almost
    # unusable while a compilation is running.
    set(terminal_args USES_TERMINAL_BUILD TRUE)
    if(CMAKE_GENERATOR MATCHES "Ninja")
        option(QT_BUILD_EXAMPLES_WITH_CPU_OVERCOMMIT
            "Allow CPU over-commitment when building examples (Ninja only)"
        )
        if(QT_BUILD_EXAMPLES_WITH_CPU_OVERCOMMIT)
            set(terminal_args)
        endif()
    endif()

    # QT_EXAMPLE_INSTALL_MARKER
    # The goal is to install each example project into a directory that keeps the example source dir
    # hierarchy, without polluting the example projects with dirty INSTALL_EXAMPLEDIR and
    # INSTALL_EXAMPLESDIR usage.
    # E.g. ensure qtbase/examples/widgets/widgets/wiggly is installed to
    # $qt_example_install_prefix/examples/widgets/widgets/wiggly/wiggly.exe
    # $qt_example_install_prefix defaults to ${CMAKE_INSTALL_PREFIX}/${INSTALL_EXAMPLEDIR}
    # but can also be set to a custom location.
    # This needs to work both:
    #  - when using ExternalProject to build examples
    #  - when examples are built in-tree as part of Qt (no ExternalProject).
    # The reason we want to support the latter is for nicer IDE integration: a can developer can
    # work with a Qt repo and its examples using the same build dir.
    #
    # In both case we have to ensure examples are not accidentally installed to $qt_prefix/bin or
    # similar.
    #
    # Example projects installation matrix.
    # 1) ExternalProject + unclean example install rules (INSTALL_EXAMPLEDIR is set) =>
    #    use _qt_internal_override_example_install_dir_to_dot + ExternalProject_Add's INSTALL_DIR
    #    using relative_dir from QT_EXAMPLE_BASE_DIR to example_source_dir
    #
    # 2) ExternalProject + clean example install rules =>
    #    use ExternalProject_Add's INSTALL_DIR using relative_dir from QT_EXAMPLE_BASE_DIR to
    #    example_source_dir, _qt_internal_override_example_install_dir_to_dot would be a no-op
    #
    # 3) in-tree + unclean example install rules (INSTALL_EXAMPLEDIR is set)
    # +
    # 4) in-tree + clean example install rules =>
    #    ensure CMAKE_INSTALL_PREFIX is unset in parent cmake_install.cmake file, set non-cache
    #    CMAKE_INSTALL_PREFIX using relative_dir from QT_EXAMPLE_BASE_DIR to
    #    example_source_dir, use _qt_internal_override_example_install_dir_to_dot to ensure
    #    INSTALL_EXAMPLEDIR does not interfere.

    qt_internal_get_example_install_path(example_install_path "${subdir}")

    set(ep_binary_dir    "${CMAKE_CURRENT_BINARY_DIR}/${subdir}")

    set(build_command "")
    if(QT_INTERNAL_VERBOSE_EXAMPLES AND CMAKE_GENERATOR MATCHES "Ninja")
        set(build_command BUILD_COMMAND "${CMAKE_COMMAND}" --build "." -- -v)
    endif()

    ExternalProject_Add(${arg_NAME}
        EXCLUDE_FROM_ALL TRUE
        SOURCE_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}"
        PREFIX           "${CMAKE_CURRENT_BINARY_DIR}/${subdir}-ep"
        STAMP_DIR        "${CMAKE_CURRENT_BINARY_DIR}/${subdir}-ep/stamp"
        BINARY_DIR       "${ep_binary_dir}"
        INSTALL_DIR      "${example_install_path}"
        INSTALL_COMMAND  ""
        ${build_command}
        TEST_COMMAND     ""
        DEPENDS          ${deps}
        CMAKE_CACHE_ARGS ${var_defs}
                         -DQT_INTERNAL_IS_EXAMPLE_EP_BUILD:BOOL=TRUE
                         -DCMAKE_INSTALL_PREFIX:STRING=<INSTALL_DIR>
                         -DQT_INTERNAL_SET_EXAMPLE_INSTALL_DIR_TO_DOT:BOOL=TRUE
        ${terminal_args}
    )

    # Install the examples when the the user runs 'make install', and not at build time (which is
    # the default for ExternalProjects).
    install(CODE "\
# Install example from inside ExternalProject into the main build's install prefix.
execute_process(
    COMMAND
        \"${CMAKE_COMMAND}\" --build \"${ep_binary_dir}\" --target install
)
")

    # Force configure step to re-run after we configure the main project
    set(reconfigure_check_file ${CMAKE_CURRENT_BINARY_DIR}/reconfigure_${arg_NAME}.txt)
    file(TOUCH ${reconfigure_check_file})
    ExternalProject_Add_Step(${arg_NAME} reconfigure-check
        DEPENDERS configure
        DEPENDS   ${reconfigure_check_file}
        ${independent_args}
    )

    # Create an apk external project step and custom target that invokes the apk target
    # within the external project.
    # Make the global apk target depend on that custom target.
    if(ANDROID)
        ExternalProject_Add_Step(${arg_NAME} apk
            COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target apk
            DEPENDEES configure
            EXCLUDE_FROM_MAIN YES
            ${terminal_args}
        )
        ExternalProject_Add_StepTargets(${arg_NAME} apk)

        if(TARGET apk)
            add_dependencies(apk ${arg_NAME}-apk)
        endif()
    endif()

    cmake_policy(POP)

    string(TOLOWER ${PROJECT_NAME} project_name_lower)
    add_dependencies(examples_${project_name_lower} ${arg_NAME})

endfunction()

function(qt_internal_install_example_sources subdir)
    set(options "")
    set(single_args NAME REPO_NAME)
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${single_args}" "${multi_args}")

    qt_internal_get_examples_sources_install_path(example_install_path "${subdir}")

    # The trailing slash is important to avoid duplicate nested directory names.
    set(example_source_dir "${subdir}/")

    # Allow controlling whether sources should be part of the default install target.
    if(QT_INSTALL_EXAMPLES_SOURCES_BY_DEFAULT)
        set(exclude_from_all "")
    else()
        set(exclude_from_all "EXCLUDE_FROM_ALL")
    endif()

    # Create an install component for all example sources. Can also be part of the default
    # install target if EXCLUDE_FROM_ALL is not passed.
    install(
        DIRECTORY "${example_source_dir}"
        DESTINATION "${example_install_path}"
        COMPONENT "examples_sources"
        USE_SOURCE_PERMISSIONS
        ${exclude_from_all}
    )

    # Also create a specific install component just for this repo's examples.
    install(
        DIRECTORY "${example_source_dir}"
        DESTINATION "${example_install_path}"
        COMPONENT "examples_sources_${arg_REPO_NAME}"
        USE_SOURCE_PERMISSIONS
        EXCLUDE_FROM_ALL
    )

    # Also create a specific install component just for the current example's sources.
    install(
        DIRECTORY "${example_source_dir}"
        DESTINATION "${example_install_path}"
        COMPONENT "examples_sources_${arg_NAME}"
        USE_SOURCE_PERMISSIONS
        EXCLUDE_FROM_ALL
    )
endfunction()
