# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Defines the public Qt::Platform target, which serves as a dependency for all internal Qt target
# as well as user projects consuming Qt.
function(qt_internal_setup_public_platform_target)
    qt_internal_get_platform_definition_include_dir(
        install_interface_definition_dir
        build_interface_definition_dir
    )

    ## QtPlatform Target:
    qt_internal_add_platform_target(Platform)
    target_include_directories(Platform
        INTERFACE
        $<BUILD_INTERFACE:${build_interface_definition_dir}>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${install_interface_definition_dir}>
        $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>
        )
    target_compile_definitions(Platform INTERFACE ${QT_PLATFORM_DEFINITIONS})

    set_target_properties(Platform PROPERTIES
        _qt_package_version "${PROJECT_VERSION}"
    )
    set_property(TARGET Platform
                 APPEND PROPERTY
                 EXPORT_PROPERTIES "_qt_package_version")

    # When building on android we need to link against the logging library
    # in order to satisfy linker dependencies. Both of these libraries are part of
    # the NDK.
    if (ANDROID)
        if(QT_FEATURE_android_16kb_pages)
            target_link_options(Platform INTERFACE "-Wl,-z,max-page-size=16384")
        endif()
        target_link_libraries(Platform INTERFACE log)
    endif()

    if (QT_FEATURE_stdlib_libcpp)
        target_compile_options(Platform INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>")
        set(libc_link_option "-stdlib=libc++")
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
            set(libc_link_option "$<$<LINK_LANGUAGE:CXX>:-stdlib=libc++>")
        endif()
        target_link_options(Platform INTERFACE "${libc_link_option}")
    endif()
    if(QT_FEATURE_no_direct_extern_access)
        # Swift compiler doesn't understand these options on Linux.
        set(not_swift_cond "$<NOT:$<COMPILE_LANGUAGE:Swift>>")

        set(no_direct_extern_gnu "$<$<CXX_COMPILER_ID:GNU>:-mno-direct-extern-access>")
        set(no_direct_extern_gnu_wrapped "$<${not_swift_cond}:${no_direct_extern_gnu}>")

        set(no_direct_extern_clang "$<$<CXX_COMPILER_ID:Clang>:-fno-direct-access-external-data>")
        set(no_direct_extern_clang_wrapped "$<${not_swift_cond}:${no_direct_extern_clang}>")

        target_compile_options(Platform INTERFACE "${no_direct_extern_gnu_wrapped}")
        target_compile_options(Platform INTERFACE "${no_direct_extern_clang_wrapped}")
    endif()

    qt_set_msvc_cplusplus_options(Platform INTERFACE)

    # Propagate minimum C++ 17 via Platform to Qt consumers (apps), after the global features
    # are computed.
    qt_set_language_standards_interface_compile_features(Platform)

    # By default enable utf8 sources for both Qt and Qt consumers. Can be opted out.
    qt_enable_utf8_sources(Platform)

    # By default enable unicode on WIN32 platforms for both Qt and Qt consumers. Can be opted out.
    qt_internal_enable_unicode_defines(Platform)

    # Generate a pkgconfig for Qt::Platform.
    qt_internal_generate_pkg_config_file(Platform)

    qt_internal_add_sbom(Platform
        SBOM_ENTITY_TYPE QT_MODULE
        ATTRIBUTION_FILE_DIR_PATHS
            "${PROJECT_SOURCE_DIR}/cmake/3rdparty/extra-cmake-modules"
            "${PROJECT_SOURCE_DIR}/cmake/3rdparty/kwin"
    )
endfunction()

function(qt_internal_get_platform_definition_include_dir install_interface build_interface)
    # Used by consumers of prefix builds via INSTALL_INTERFACE (relative path).
    set(${install_interface} "${INSTALL_MKSPECSDIR}/${QT_QMAKE_TARGET_MKSPEC}" PARENT_SCOPE)

    # Used by qtbase in prefix builds via BUILD_INTERFACE
    set(build_interface_base_dir
        "${CMAKE_CURRENT_LIST_DIR}/../mkspecs"
    )

    # Used by qtbase and consumers in non-prefix builds via BUILD_INTERFACE
    if(NOT QT_WILL_INSTALL)
        set(build_interface_base_dir
            "${QT_BUILD_DIR}/${INSTALL_MKSPECSDIR}"
        )
    endif()

    get_filename_component(build_interface_dir
        "${build_interface_base_dir}/${QT_QMAKE_TARGET_MKSPEC}"
        ABSOLUTE
    )
    set(${build_interface} "${build_interface_dir}" PARENT_SCOPE)
endfunction()
