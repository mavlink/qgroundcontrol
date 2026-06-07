# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_add_resource target resourceName)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "${target} is not a target.")
    endif()
    qt_internal_is_skipped_test(skipped ${target})
    if(skipped)
        return()
    endif()
    qt_internal_is_in_test_batch(in_batch ${target})
    if(in_batch)
        _qt_internal_test_batch_target_name(target)
    endif()

    # Don't try to add resources when cross compiling, and the target is actually a host target
    # (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    cmake_parse_arguments(PARSE_ARGV 2 arg
        ""
        "PREFIX;LANG;BASE;OUTPUT_TARGETS"
        "FILES")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_process_resource(${target} ${resourceName}
        PREFIX "${arg_PREFIX}"
        LANG "${arg_LANG}"
        BASE "${arg_BASE}"
        FILES ${arg_FILES}
        OUTPUT_TARGETS out_targets
   )

   if (out_targets)
        qt_install(TARGETS ${out_targets}
            EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
            DESTINATION "${INSTALL_LIBDIR}"
        )
        qt_internal_add_targets_to_additional_targets_export_file(
            TARGETS ${out_targets}
            EXPORT_NAME_PREFIX "${INSTALL_CMAKE_NAMESPACE}${target}"
        )

        qt_internal_install_resource_pdb_files("${out_targets}")
        qt_internal_record_rcc_object_files("${target}" "${out_targets}"
                                            INSTALL_DIRECTORY "${INSTALL_LIBDIR}")
   endif()

   if (arg_OUTPUT_TARGETS)
       set(${arg_OUTPUT_TARGETS} "${out_targets}" PARENT_SCOPE)
   endif()
endfunction()

function(qt_internal_record_rcc_object_files target resource_targets)
    set(args_optional "")
    set(args_single INSTALL_DIRECTORY)
    set(args_multi "")

    cmake_parse_arguments(arg
       "${args_optional}"
       "${args_single}"
       "${args_multi}"
       ${ARGN}
    )

    foreach(out_target ${resource_targets})
        get_target_property(resource_name ${out_target} _qt_resource_name)
        if(NOT resource_name)
            continue()
        endif()
        if(QT_WILL_INSTALL)
            # Compute the install location of a resource object file in a prefix build.
            # It's comprised of thee following path parts:
            #
            # part (1) INSTALL_DIRECTORY.
            #          A usual value is '${INSTALL_LIBDIR}/' for libraries
            #          and '${INSTALL_QMLDIR}/foo/bar/' for qml plugin resources.
            #
            # part (2) the value computed by CMake's computeInstallObjectDir comprised of an
            #          objects-<CONFIG> dir and the target name of the object library.
            #          Example: objects-$<CONFIG>/Gui_resources_qpdf
            #
            # part (3) path to the object file, relative to it's build directory.
            #          Example: .rcc/qrc_qpdf.cpp.o
            #
            # The final path is relative to CMAKE_INSTALL_PREFIX aka $qt_install_prefix.
            #
            # The relative path will be transformed into an absolute path when generating .prl
            # files, by prepending $$[QT_INSTALL_PREFIX]/.
            get_target_property(generated_cpp_file_relative_path
                                ${out_target}
                                _qt_resource_generated_cpp_relative_path)

            set(object_file_name "${generated_cpp_file_relative_path}${CMAKE_CXX_OUTPUT_EXTENSION}")
            qt_path_join(rcc_object_file_path
                "objects-$<CONFIG>" ${out_target} "${object_file_name}")
            if(arg_INSTALL_DIRECTORY)
                qt_path_join(rcc_object_file_path
                             "${arg_INSTALL_DIRECTORY}" "${rcc_object_file_path}")
            else()
                message(FATAL_ERROR "No install location given for object files to be installed"
                                    " for the following resource target: '${out_target}'")
            endif()
        else()
            # In a non-prefix build we use the object file paths right away.
            set(rcc_object_file_path $<TARGET_OBJECTS:$<TARGET_NAME:${out_target}>>)
        endif()
        set_property(TARGET ${target} APPEND PROPERTY _qt_rcc_objects "${rcc_object_file_path}")

        qt_internal_link_internal_platform_for_object_library("${out_target}"
            PARENT_TARGET "${target}")
    endforeach()
endfunction()

function(qt_internal_install_resource_pdb_files objlib_targets)
    if(NOT MSVC OR NOT QT_WILL_INSTALL)
        return()
    endif()

    foreach(target IN LISTS objlib_targets)
        qt_internal_set_compile_pdb_names(${target})

        get_target_property(generated_cpp_file_relative_path
            ${target}
            _qt_resource_generated_cpp_relative_path)
        get_filename_component(rel_obj_file_dir "${generated_cpp_file_relative_path}" DIRECTORY)
        qt_internal_install_pdb_files(${target}
            "${INSTALL_LIBDIR}/objects-$<CONFIG>/${target}/${rel_obj_file_dir}")
    endforeach()
endfunction()
