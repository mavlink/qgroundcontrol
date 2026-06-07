# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapBrotli::WrapBrotliDec)
    set(WrapBrotli_FOUND ON)
    return()
endif()

# From VCPKG
find_package(unofficial-brotli CONFIG QUIET)
if (unofficial-brotli_FOUND)
    add_library(WrapBrotli::WrapBrotliDec INTERFACE IMPORTED)
    target_link_libraries(WrapBrotli::WrapBrotliDec INTERFACE unofficial::brotli::brotlidec)

    add_library(WrapBrotli::WrapBrotliEnc INTERFACE IMPORTED)
    target_link_libraries(WrapBrotli::WrapBrotliEnc INTERFACE unofficial::brotli::brotlienc)

    add_library(WrapBrotli::WrapBrotliCommon INTERFACE IMPORTED)
    target_link_libraries(WrapBrotli::WrapBrotliCommon INTERFACE unofficial::brotli::brotlicommon)

    set(WrapBrotli_FOUND ON)
else()
    get_cmake_property(__packages_not_found PACKAGES_NOT_FOUND)
    if(__packages_not_found)
        list(REMOVE_ITEM __packages_not_found unofficial-brotli)
        set_property(GLOBAL PROPERTY PACKAGES_NOT_FOUND "${__packages_not_found}")
    endif()
    unset(__packages_not_found)

    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(libbrotlidec QUIET IMPORTED_TARGET "libbrotlidec")
        if (libbrotlidec_FOUND)
            add_library(WrapBrotli::WrapBrotliDec INTERFACE IMPORTED)
            target_link_libraries(WrapBrotli::WrapBrotliDec INTERFACE PkgConfig::libbrotlidec)
            set(WrapBrotli_FOUND ON)
        endif()

        pkg_check_modules(libbrotlienc QUIET IMPORTED_TARGET "libbrotlienc")
        if (libbrotlienc_FOUND)
            add_library(WrapBrotli::WrapBrotliEnc INTERFACE IMPORTED)
            target_link_libraries(WrapBrotli::WrapBrotliEnc INTERFACE PkgConfig::libbrotlienc)
            set(WrapBrotli_FOUND ON)
        endif()

        pkg_check_modules(libbrotlicommon QUIET IMPORTED_TARGET "libbrotlicommon")
        if (libbrotlicommon_FOUND)
            add_library(WrapBrotli::WrapBrotliCommon INTERFACE IMPORTED)
            target_link_libraries(WrapBrotli::WrapBrotliCommon INTERFACE PkgConfig::libbrotlicommon)
            set(WrapBrotli_FOUND ON)
        endif()
    else()
        find_path(BROTLI_INCLUDE_DIR NAMES "brotli/decode.h")

        foreach(lib_name BrotliDec BrotliEnc BrotliCommon)
            string(TOLOWER ${lib_name} lower_lib_name)

            find_library(${lib_name}_LIBRARY_RELEASE
                         NAMES ${lower_lib_name} ${lower_lib_name}-static)

            find_library(${lib_name}_LIBRARY_DEBUG
                         NAMES ${lower_lib_name}d ${lower_lib_name}-staticd
                         ${lower_lib_name} ${lower_lib_name}-static)

            include(SelectLibraryConfigurations)
            select_library_configurations(${lib_name})

            if (BROTLI_INCLUDE_DIR AND ${lib_name}_LIBRARY)
                set(${lib_name}_FOUND TRUE)
            endif()

            if (${lib_name}_FOUND AND NOT TARGET WrapBrotli::Wrap${lib_name})
                add_library(WrapBrotli::Wrap${lib_name} UNKNOWN IMPORTED)
                set_target_properties(WrapBrotli::Wrap${lib_name} PROPERTIES
                                      INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIR}"
                                      IMPORTED_LOCATION "${${lib_name}_LIBRARY}")

                if(${lib_name}_LIBRARY_RELEASE)
                    foreach(config_name RELEASE RELWITHDEBINFO MINSIZEREL)
                        set_property(TARGET WrapBrotli::Wrap${lib_name} APPEND PROPERTY
                                     IMPORTED_CONFIGURATIONS ${config_name})
                        set_target_properties(WrapBrotli::Wrap${lib_name} PROPERTIES
                                              IMPORTED_LOCATION_${config_name} "${${lib_name}_LIBRARY_RELEASE}")
                    endforeach()
                endif()

                if(${lib_name}_LIBRARY_DEBUG)
                    set_property(TARGET WrapBrotli::Wrap${lib_name} APPEND PROPERTY
                                 IMPORTED_CONFIGURATIONS DEBUG)
                    set_target_properties(WrapBrotli::Wrap${lib_name} PROPERTIES
                                          IMPORTED_LOCATION_DEBUG "${${lib_name}_LIBRARY_DEBUG}")
                endif()
            endif()
        endforeach()

        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(WrapBrotli REQUIRED_VARS
                                          BrotliDec_FOUND BrotliEnc_FOUND BrotliCommon_FOUND)

        if (WrapBrotli_FOUND)
            set_property(TARGET WrapBrotli::WrapBrotliDec APPEND PROPERTY
                         INTERFACE_LINK_LIBRARIES WrapBrotli::WrapBrotliCommon)
            set_property(TARGET WrapBrotli::WrapBrotliEnc APPEND PROPERTY
                         INTERFACE_LINK_LIBRARIES WrapBrotli::WrapBrotliCommon)
        endif()
    endif()
endif()
