# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Collects the library dependencies of a target.
# As well as rcc object file dependencies.
# This takes into account transitive usage requirements.
function(qt_collect_libs target libs_out_var rcc_objects_out_var)
    __qt_internal_walk_libs("${target}" "${libs_out_var}"
                            "${rcc_objects_out_var}" "qt_collect_libs_dict" "collect_libs")
    set("${libs_out_var}" "${${libs_out_var}}" PARENT_SCOPE)

    set(${rcc_objects_out_var} "${${rcc_objects_out_var}}" PARENT_SCOPE)

endfunction()

# Generate a qmake .prl file for the given target.
# The install_dir argument is a relative path, for example "lib".
function(qt_generate_prl_file target install_dir)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    unset(prl_config)
    set(is_static FALSE)
    if(target_type STREQUAL "STATIC_LIBRARY")
        list(APPEND prl_config static)
        set(is_static TRUE)
    elseif(target_type STREQUAL "SHARED_LIBRARY")
        list(APPEND prl_config shared)
    endif()
    if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(is_fw ${target} FRAMEWORK)
        if(is_fw)
            list(APPEND prl_config lib_bundle)
        endif()
    endif()
    list(JOIN prl_config " " prl_config)

    set(rcc_objects "")
    set(prl_step1_content_libs "")
    if(NOT is_static AND WIN32)
        # Do nothing. Prl files for shared libraries on Windows shouldn't have the libs listed,
        # as per qt_build_config.prf and the conditional CONFIG+=explicitlib assignment.
    else()
        set(prl_libs "")
        qt_collect_libs(${target} prl_libs prl_rcc_objects)
        if(prl_libs)
            set(prl_step1_content_libs "QMAKE_PRL_LIBS_FOR_CMAKE = ${prl_libs}\n")
        endif()
        if(prl_rcc_objects)
            list(APPEND rcc_objects ${prl_rcc_objects})
        endif()
    endif()

    if(rcc_objects AND QT_WILL_INSTALL)
        list(TRANSFORM rcc_objects PREPEND "$$[QT_INSTALL_PREFIX]/")
    endif()

    # Generate a preliminary .prl file that contains absolute paths to all libraries
    if(MINGW)
        # For MinGW, qmake doesn't have a lib prefix in prl files.
        set(prefix_for_final_prl_name "")
    else()
        set(prefix_for_final_prl_name "$<TARGET_FILE_PREFIX:${target}>")
    endif()

    # For macOS frameworks, the prl file should be placed under the Resources subdir.
    # For iOS, visionOS, watchOS, tvOS, there is no Resources subdir, and the contents needs to
    # be placed directly in the framework root, as described at
    # https://developer.apple.com/documentation/bundleresources/placing-content-in-a-bundle?language=objc
    get_target_property(is_framework ${target} FRAMEWORK)
    if(APPLE AND (NOT CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
        set(is_macos TRUE)
    else()
        set(is_macos FALSE)
    endif()
    if(is_framework AND is_macos)
        get_target_property(fw_version ${target} FRAMEWORK_VERSION)
        string(APPEND prefix_for_final_prl_name "Versions/${fw_version}/Resources/")
    endif()

    # What follows is a complicated setup for generating configuration-specific
    # prl files. It has to be this way, because add_custom_command doesn't support
    # generator expressions in OUTPUT or DEPENDS.
    # To circumvent that, we create well known file names with file(GENERATE)
    # with configuration specific content, which are then fed to add_custom_command
    # that uses these genex-less file names. The actual command will extract the info
    # from the configuration-specific files, and create a properly named final prl file.

    # The file is named according to a pattern, that is then used in the
    # add_custom_command.
    set(prl_step1_name_prefix "preliminary_prl_for_${target}_step1_")
    set(prl_step1_name_suffix ".prl" )
    qt_path_join(prl_step1_path
                 "${CMAKE_CURRENT_BINARY_DIR}"
                 "${prl_step1_name_prefix}$<CONFIG>${prl_step1_name_suffix}")

    # Same, except instead of containing the prl contents, it will contain the final prl file
    # name computed via a generator expression.
    set(prl_meta_info_name_prefix "preliminary_prl_meta_info_for_${target}_")
    set(prl_meta_info_name_suffix ".txt")
    qt_path_join(prl_meta_info_path
                 "${CMAKE_CURRENT_BINARY_DIR}"
                 "${prl_meta_info_name_prefix}$<CONFIG>${prl_meta_info_name_suffix}")

    # The final prl file name that will be embedded in the file above.
    set(final_prl_file_name "${prefix_for_final_prl_name}$<TARGET_FILE_BASE_NAME:${target}>")
    if(ANDROID)
        string(APPEND final_prl_file_name "_${CMAKE_ANDROID_ARCH_ABI}")
    endif()
    string(APPEND final_prl_file_name ".prl")
    qt_path_join(final_prl_file_path "${QT_BUILD_DIR}/${install_dir}" "${final_prl_file_name}")

    # Generate the prl content and its final file name into configuration specific files
    # whose names we know, and can be used in add_custom_command.
    set(prl_step1_content
        "RCC_OBJECTS = ${rcc_objects}
QMAKE_PRL_TARGET = $<TARGET_LINKER_FILE_NAME:${target}>
QMAKE_PRL_TARGET_PATH_FOR_CMAKE = $<TARGET_LINKER_FILE:${target}>
QMAKE_PRL_CONFIG = ${prl_config}
QMAKE_PRL_VERSION = ${PROJECT_VERSION}
${prl_step1_content_libs}
")

    file(GENERATE
        OUTPUT "${prl_step1_path}"
        CONTENT "${prl_step1_content}")
    file(GENERATE
         OUTPUT "${prl_meta_info_path}"
         CONTENT
         "FINAL_PRL_FILE_PATH = ${final_prl_file_path}")

    set(library_prefixes ${CMAKE_SHARED_LIBRARY_PREFIX} ${CMAKE_STATIC_LIBRARY_PREFIX})
    set(library_suffixes
        ${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES}
        ${CMAKE_STATIC_LIBRARY_SUFFIX})

    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs ${CMAKE_BUILD_TYPE})
    endif()

    set(qt_lib_dirs "${QT_BUILD_DIR}/${INSTALL_LIBDIR}")
    if(QT_WILL_INSTALL)
        list(APPEND qt_lib_dirs
             "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    endif()

    set(qt_plugin_dirs "${QT_BUILD_DIR}/${INSTALL_PLUGINSDIR}")
    if(QT_WILL_INSTALL)
        list(APPEND qt_plugin_dirs
             "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_PLUGINSDIR}")
    endif()

    set(qt_qml_dirs "${QT_BUILD_DIR}/${INSTALL_QMLDIR}")
    if(QT_WILL_INSTALL)
        list(APPEND qt_qml_dirs
             "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_QMLDIR}")
    endif()

    foreach(config ${configs})
        # Output file for dependency tracking, and which will contain the final content.
        qt_path_join(prl_step2_path
                     "${CMAKE_CURRENT_BINARY_DIR}" "preliminary_prl_for_${target}_step2_${config}.prl")

        # Input dependency names that are constructed for each config manually
        # (no genexes allowed).
        qt_path_join(prl_step1_path
                     "${CMAKE_CURRENT_BINARY_DIR}"
                     "${prl_step1_name_prefix}${config}${prl_step1_name_suffix}")
        qt_path_join(prl_meta_info_path
                     "${CMAKE_CURRENT_BINARY_DIR}"
                     "${prl_meta_info_name_prefix}${config}${prl_meta_info_name_suffix}")
        if(MSVC)
            set(link_library_flag "-l")
            file(TO_CMAKE_PATH "$ENV{LIB};${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}" implicit_link_directories)
        else()
            set(link_library_flag ${CMAKE_LINK_LIBRARY_FLAG})
            set(implicit_link_directories ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
        endif()
        add_custom_command(
            OUTPUT  "${prl_step2_path}"
            DEPENDS "${prl_step1_path}"
                    "${prl_meta_info_path}"
                    "${QT_CMAKE_DIR}/QtFinishPrlFile.cmake"
                    "${QT_CMAKE_DIR}/QtGenerateLibHelpers.cmake"
            COMMAND ${CMAKE_COMMAND}
                    "-DIN_FILE=${prl_step1_path}"
                    "-DIN_META_FILE=${prl_meta_info_path}"
                    "-DOUT_FILE=${prl_step2_path}"
                    "-DLIBRARY_PREFIXES=${library_prefixes}"
                    "-DLIBRARY_SUFFIXES=${library_suffixes}"
                    "-DLINK_LIBRARY_FLAG=${link_library_flag}"
                    "-DQT_LIB_DIRS=${qt_lib_dirs}"
                    "-DQT_PLUGIN_DIRS=${qt_plugin_dirs}"
                    "-DQT_QML_DIRS=${qt_qml_dirs}"
                    "-DIMPLICIT_LINK_DIRECTORIES=${implicit_link_directories}"
                    -P "${QT_CMAKE_DIR}/QtFinishPrlFile.cmake"
            VERBATIM
            COMMENT "Generating prl file for target ${target}"
            )

        # Tell the target to depend on the preliminary prl file, to ensure the custom command
        # is executed. As a side-effect, this will also create the final prl file that
        # is named appropriately. It should not be specified as a BYPRODUCT.
        # This allows proper per-file dependency tracking, without having to resort on a POST_BUILD
        # step, which means that relinking would happen as well as transitive rebuilding of any
        # dependees.
        # This is inspired by https://gitlab.kitware.com/cmake/cmake/-/issues/20842
        target_sources(${target} PRIVATE "${prl_step2_path}")
    endforeach()

    # Install the final .prl file that's generated by the QtFinishPrlFile.cmake script.
    # For Apple frameworks, the .prl file is already placed inside the framework.
    if(NOT is_framework)
        qt_install(FILES "${final_prl_file_path}" DESTINATION "${install_dir}")
    endif()
endfunction()
