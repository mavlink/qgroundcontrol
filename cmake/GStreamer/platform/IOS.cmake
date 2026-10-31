# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# iOS GStreamer SDK discovery — invoked by Orchestrator.cmake.
# Up to 1.28.7 the iOS package is a .pkg holding a universal GStreamer.framework (cerbero versioned bundle) whose
# Versions/1.0/GStreamer binary is a single merged static archive containing every library and plugin. From 1.28.8 on
# it is a tarball holding a GStreamer.xcframework with a fat libGStreamer.a per slice. Both reduce
# to the same single-merged-archive consumption model
# (_qgc_gst_create_ios_targets) — discovery just locates the binary + Headers.
#
# The GStreamer::mobile framework target (_qgc_gst_ios_mobile_framework) is derived from FindGStreamerMobile.cmake in
# cerbero (https://gitlab.freedesktop.org/gstreamer/cerbero, data/mobile/FindGStreamerMobile.cmake). QGC changes: it is
# built from the SDK's merged static archive, registers the requested plugins found in that archive through the shared
# C registration template instead of an Objective-C one, and SDK discovery is QGC's own.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/../../modules/AppleXCFramework.cmake")

# _qgc_gst_ios_bundle_from_root(<root> <out_bundle>)
# Maps a GStreamer_ROOT_DIR to the SDK bundle, a GStreamer.xcframework or GStreamer.framework. Accepts the bundle
# itself, a directory containing one (e.g. iPhone.sdk), an xcframework slice or a framework's Versions/<v> directory;
# the last two are the roots discovery records. <out_bundle> is empty when <root> matches none of these.
function(_qgc_gst_ios_bundle_from_root root out_bundle)
    string(REGEX REPLACE "/+$" "" _root "${root}")
    cmake_path(GET _root PARENT_PATH _parent)
    cmake_path(GET _parent PARENT_PATH _grandparent)
    set(_bundle "")
    if(_root MATCHES "\\.(xc)?framework$")
        set(_bundle "${_root}")
    elseif(IS_DIRECTORY "${_root}/GStreamer.xcframework")
        set(_bundle "${_root}/GStreamer.xcframework")
    elseif(IS_DIRECTORY "${_root}/GStreamer.framework")
        set(_bundle "${_root}/GStreamer.framework")
    elseif(_parent MATCHES "\\.xcframework$")
        set(_bundle "${_parent}")
    elseif(_parent MATCHES "/Versions$" AND _grandparent MATCHES "\\.framework$")
        set(_bundle "${_grandparent}")
    endif()
    if(NOT IS_DIRECTORY "${_bundle}")
        set(_bundle "")
    endif()
    set(${out_bundle}
        "${_bundle}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_ios_layout(<bundle> <prefix>)
# Locates the merged static archive and Headers of the GStreamer.xcframework (the slice for CMAKE_OSX_SYSROOT and
# CMAKE_OSX_ARCHITECTURES) or GStreamer.framework <bundle>. Sets <prefix>_ROOT
# (the slice or the framework's Versions/<v> directory), <prefix>_LIB (the archive) and <prefix>_INCLUDE (its headers)
# in the caller's scope, failing configure when the archive, or a framework's Headers, is missing.
# cmake-lint: disable=C0103
function(_qgc_gst_ios_layout bundle prefix)
    if(bundle MATCHES "\\.xcframework$")
        # Same platform test as cmake/platform/Apple.cmake: CMAKE_OSX_SYSROOT is a name or an SDK path.
        if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
            set(_platform iphonesimulator)
        else()
            set(_platform iphoneos)
        endif()
        set(_slice_args XCFRAMEWORK "${bundle}" PLATFORM ${_platform} OUT_VAR _root)
        if(CMAKE_OSX_ARCHITECTURES)
            list(APPEND _slice_args ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
        endif()
        qgc_find_ios_xcframework_slice(${_slice_args})
        set(_lib "${_root}/libGStreamer.a")
        set(_include "${_root}/Headers")
    else()
        # cerbero universal .framework: one merged fat binary covers all arches.
        # Versioned bundle (Versions/1.0/...) is canonical; tolerate a flat bundle.
        if(EXISTS "${bundle}/Versions/1.0/GStreamer")
            set(_root "${bundle}/Versions/1.0")
        elseif(EXISTS "${bundle}/GStreamer")
            set(_root "${bundle}")
        else()
            message(
                FATAL_ERROR "GStreamer.framework at ${bundle} has no merged binary at "
                            "Versions/1.0/GStreamer or GStreamer. The SDK layout may have changed."
            )
        endif()
        set(_lib "${_root}/GStreamer")
        # Headers may live under Versions/1.0/Headers or be a symlink at the root.
        if(EXISTS "${_root}/Headers")
            set(_include "${_root}/Headers")
        elseif(EXISTS "${bundle}/Headers")
            set(_include "${bundle}/Headers")
        else()
            message(FATAL_ERROR "GStreamer.framework at ${bundle} has no Headers/ directory.")
        endif()
    endif()
    if(NOT EXISTS "${_lib}")
        message(FATAL_ERROR "GStreamer iOS merged binary not found at ${_lib}")
    endif()
    set(${prefix}_ROOT
        "${_root}"
        PARENT_SCOPE
    )
    set(${prefix}_LIB
        "${_lib}"
        PARENT_SCOPE
    )
    set(${prefix}_INCLUDE
        "${_include}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_ios_sdk_version(<include_dir> <out_var>)
# Sets <out_var> to the version gst/gstversion.h under <include_dir> defines, which works for every SDK layout, and
# fails configure when it is older than QGC_GST_REQUESTED_VERSION, as the pkg-config platforms' version check does.
# Without the header it warns and falls back to QGC_GST_REQUESTED_VERSION.
function(_qgc_gst_ios_sdk_version include_dir out_var)
    _qgc_gst_header_version("${include_dir}" _version)
    if(_version)
        if(_version VERSION_LESS QGC_GST_REQUESTED_VERSION)
            message(
                FATAL_ERROR
                    "GStreamer iOS SDK headers (${include_dir}/gst/gstversion.h) report version '${_version}', "
                    "older than the requested ${QGC_GST_REQUESTED_VERSION}. Point GStreamer_ROOT_DIR at an SDK of "
                    "that version, or unset it and clear the SDK cache to download one."
            )
        endif()
    else()
        message(
            WARNING
                "GStreamer iOS: could not read GST_VERSION_* from ${include_dir}/gst/gstversion.h. Falling "
                "back to the REQUESTED version '${QGC_GST_REQUESTED_VERSION}' — the SDK's actual version is "
                "UNVERIFIED, so a downgraded SDK cannot be detected."
        )
        set(_version "${QGC_GST_REQUESTED_VERSION}")
    endif()
    set(${out_var}
        "${_version}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_discover_ios_sdk()
# Uses the GStreamer.xcframework or GStreamer.framework at or under GStreamer_ROOT_DIR, else the system iPhone.sdk,
# else downloads the official iOS package, and selects its single merged static archive; there is no pkg-config. Also
# sets GSTREAMER_XCFRAMEWORK_LIB and GStreamer_IOS_CA_BUNDLE.
# cmake-lint: disable=C0103
function(_qgc_gst_discover_ios_sdk)
    if(NOT CMAKE_HOST_APPLE)
        message(FATAL_ERROR "GStreamer for iOS can only be built on macOS")
    endif()

    if(GStreamer_ROOT_DIR)
        _qgc_gst_ios_bundle_from_root("${GStreamer_ROOT_DIR}" _gst_ios_bundle)
        if(NOT _gst_ios_bundle)
            message(
                FATAL_ERROR "GStreamer: GStreamer_ROOT_DIR '${GStreamer_ROOT_DIR}' is not a GStreamer.xcframework or "
                            "GStreamer.framework, or a directory containing one."
            )
        endif()
    else()
        _qgc_gst_ios_bundle_from_root("/Library/Developer/GStreamer/iPhone.sdk" _gst_ios_bundle)
    endif()

    if(NOT _gst_ios_bundle)
        _qgc_gst_acquire_ios_sdk(_gst_ios_expanded)
        # The package's bundle: the 1.28.8+ tarball's GStreamer.xcframework, else the older .pkg's GStreamer.framework.
        file(
            GLOB_RECURSE _gst_ios_bundles
            LIST_DIRECTORIES true
            "${_gst_ios_expanded}/*"
        )
        list(FILTER _gst_ios_bundles INCLUDE REGEX "/GStreamer\\.(xc)?framework$")
        set(_gst_ios_xcframeworks ${_gst_ios_bundles})
        list(FILTER _gst_ios_xcframeworks INCLUDE REGEX "xcframework$")
        list(PREPEND _gst_ios_bundles ${_gst_ios_xcframeworks})
        if(NOT _gst_ios_bundles)
            message(
                FATAL_ERROR "GStreamer: no GStreamer.xcframework or GStreamer.framework in the expanded iOS SDK at "
                            "${_gst_ios_expanded}; the package layout may have changed."
            )
        endif()
        list(GET _gst_ios_bundles 0 _gst_ios_bundle)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    # Both kinds are consumed as one merged static archive (_qgc_gst_create_ios_targets).
    _qgc_gst_ios_layout("${_gst_ios_bundle}" _gst_ios)
    set(GStreamer_ROOT_DIR "${_gst_ios_ROOT}")
    set(GSTREAMER_XCFRAMEWORK_LIB "${_gst_ios_LIB}")

    # The merged archive's directory holds the whole SDK: there is no lib/ or plugin directory.
    set(GSTREAMER_LIB_PATH "${_gst_ios_ROOT}")
    set(GSTREAMER_PLUGIN_PATH "${_gst_ios_ROOT}")
    set(GSTREAMER_INCLUDE_PATH "${_gst_ios_INCLUDE}")

    # iOS SDK ships no CA bundle reachable from libgioopenssl; fetch Mozilla NSS.
    _qgc_gst_download_ios_ca_bundle()
    _qgc_gst_return_discovery()
endfunction()

# Downloads ca-certificates.crt into CPM_SOURCE_CACHE and sets GStreamer_IOS_CA_BUNDLE to it in the caller's scope.
function(_qgc_gst_download_ios_ca_bundle)
    set(_ca_dir "${CPM_SOURCE_CACHE}/gstreamer-ios-ca")
    # The bundle becomes the app's TLS trust root, so it is only fetched against its pinned checksum.
    qgc_config_read(_ca_url gstreamer ca_bundle_url)
    qgc_config_read(_ca_sha256 gstreamer ca_bundle_sha256)
    qgc_resilient_download(
        FILENAME
        ca-certificates.crt
        DESTINATION_DIR
        "${_ca_dir}"
        URLS
        "${_ca_url}"
        RESULT_VAR
        _ca_path
        LOG_TAG
        "iOS CA bundle"
        FAILURE_HINT
        "Network is required at first iOS configure to fetch the Mozilla CA bundle from curl.se."
        EXPECTED_HASH
        "SHA256=${_ca_sha256}"
    )

    set(GStreamer_IOS_CA_BUNDLE
        "${_ca_path}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_ios_select_static_plugins(<descriptors> <requested> <registered_out>)
# Sets <registered_out> to the <requested> plugins, in order, whose descriptor symbol (_gst_plugin_<name>_get_desc, as
# nm lists it for the merged archive) is in <descriptors>.
function(_qgc_gst_ios_select_static_plugins descriptors requested registered_out)
    list(TRANSFORM descriptors REPLACE "^_gst_plugin_(.+)_get_desc$" "\\1" OUTPUT_VARIABLE _archived)
    set(_registered "")
    foreach(_plugin IN LISTS requested)
        if(_plugin IN_LIST _archived)
            list(APPEND _registered "${_plugin}")
        endif()
    endforeach()
    set(${registered_out}
        "${_registered}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_ios_usage_targets(<archive> <include_dir>)
# Creates GStreamer_static, the merged <archive>, and GStreamer::GStreamer with only the usage requirements: the headers
# under <include_dir> (_qgc_gst_create_ios_targets adds the system frameworks and link options). Only GStreamer::mobile
# links the archive (_qgc_gst_ios_mobile_framework): a GStreamer function QGC calls that the framework does not export
# then fails the app's link, naming the symbol, instead of linking a second copy of its object, and possibly a second
# core, from the archive into the app.
function(_qgc_gst_ios_usage_targets archive include_dir)
    add_library(GStreamer_static STATIC IMPORTED GLOBAL)
    set_target_properties(
        GStreamer_static
        PROPERTIES
            IMPORTED_LOCATION "${archive}"
    )
    add_library(GStreamer::GStreamer INTERFACE IMPORTED GLOBAL)
    # iOS .framework has flat Headers/; macOS-style installs have the gstreamer-1.0 /
    # glib-2.0 subdirs. Add only what exists — INTERFACE_INCLUDE_DIRECTORIES is validated.
    target_include_directories(GStreamer::GStreamer INTERFACE "${include_dir}")
    foreach(_inc_sub IN ITEMS gstreamer-1.0 glib-2.0)
        if(IS_DIRECTORY "${include_dir}/${_inc_sub}")
            target_include_directories(GStreamer::GStreamer INTERFACE "${include_dir}/${_inc_sub}")
        endif()
    endforeach()
endfunction()

# _qgc_gst_ios_mobile_framework(<plugin>...)
# Creates GStreamer::mobile (qgc_gst_add_mobile_library) as the framework the app embeds and the only binary that links
# GStreamer_static. Its generated gst_init_static_plugins() registers the <plugin>s and loads G_IO_MODULES, which is
# what links their objects in; the core functions in QGC_GST_APP_CORE_SYMBOLS (Mobile.cmake) that no plugin pulls
# in are kept. The app resolves every GStreamer symbol from this framework.
function(_qgc_gst_ios_mobile_framework)
    # gioopenssl is statically linked into libGStreamer.a (nm: _g_io_openssl_load is exported from the slice);
    # loading it registers the default GIO TLS backend.
    qgc_gst_add_mobile_library(PLUGINS ${ARGN})
    target_link_libraries(
        GStreamerMobile
        PRIVATE
            GStreamer_static
            GStreamer::GStreamer
    )
    set_target_properties(
        GStreamerMobile
        PROPERTIES
            LIBRARY_OUTPUT_NAME gstreamer_mobile
            FRAMEWORK TRUE
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.gstreamer.GStreamerMobile
    )
endfunction()

# _qgc_gst_create_ios_targets()
# Creates the GStreamer:: targets and GStreamer::mobile from the merged archive GSTREAMER_XCFRAMEWORK_LIB, without
# pkg-config, and sets GStreamer_VERSION and GSTREAMER_STATIC_PLUGINS in the caller's scope. Every target comes from
# that one archive, so the steps share its state in one function.
# cmake-lint: disable=R0912,R0915
function(_qgc_gst_create_ios_targets)
    _qgc_gst_ios_sdk_version("${GSTREAMER_INCLUDE_PATH}" GStreamer_VERSION)
    set(GStreamer_VERSION
        "${GStreamer_VERSION}"
        PARENT_SCOPE
    )

    _qgc_gst_ios_usage_targets("${GSTREAMER_XCFRAMEWORK_LIB}" "${GSTREAMER_INCLUDE_PATH}")
    # System frameworks required by GStreamer on iOS.
    # Note: gstreamer-ios 1.28+ bundles libass (CoreText), MoltenVK (Metal/IOSurface/QuartzCore),
    # applemedia iosassetsrc (AssetsLibrary), and EAGL/CAMetalLayer (QuartzCore) — all of which
    # demand additional system frameworks at the *consumer* link step.
    set(_xcfw_required_frameworks
        Foundation
        AVFoundation
        AudioToolbox
        VideoToolbox
        CoreMedia
        CoreVideo
        CoreAudio
        CoreGraphics
        Security
        UIKit
        CoreFoundation
        CoreText
        IOSurface
        Metal
        QuartzCore
    )
    set(_xcfw_resolved_libs)
    foreach(_fw IN LISTS _xcfw_required_frameworks)
        string(TOLOWER "_xcfw_${_fw}" _fw_var)
        find_library(${_fw_var} ${_fw} REQUIRED)
        list(APPEND _xcfw_resolved_libs "${${_fw_var}}")
    endforeach()
    # AssetsLibrary is deprecated since iOS 9 but the headers/lib are still present;
    # libgstapplemedia iosassetsrc references _OBJC_CLASS_$_ALAssetsLibrary.
    find_library(GStreamer_xcfw_assetslibrary AssetsLibrary)
    set(_xcfw_assetslibrary "${GStreamer_xcfw_assetslibrary}")
    target_link_libraries(
        GStreamer::GStreamer
        INTERFACE
            ${_xcfw_resolved_libs}
            "-lresolv"
            "-liconv"
            "-lz"
            "-lbz2"
    )
    if(_xcfw_assetslibrary)
        target_link_libraries(GStreamer::GStreamer INTERFACE "${_xcfw_assetslibrary}")
    else()
        # If AssetsLibrary truly isn't present in the SDK, weak-link by name so dyld
        # tolerates absence at load time (iosassetsrc is not invoked by QGC).
        target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-weak_framework,AssetsLibrary")
    endif()
    # OpenGLES is absent from pure-Metal SDK slices; weak-link rather than hard-require.
    find_library(GStreamer_xcfw_opengles OpenGLES)
    set(_xcfw_opengles "${GStreamer_xcfw_opengles}")
    if(_xcfw_opengles)
        target_link_libraries(GStreamer::GStreamer INTERFACE "${_xcfw_opengles}")
    else()
        target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-weak_framework,OpenGLES")
    endif()
    # The archive's Rust plugins each bring a _rust_eh_personality, more than Mach-O compact unwind can encode; use
    # DWARF unwind tables instead (cerbero adds the same flag through pkg-config).
    target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-no_compact_unwind")

    # GStreamer 1.28+ compiles every plugin into the merged archive but provides no registration entry point.
    # Enumerate the plugin descriptors in the archive and register the requested ones: registration is what links
    # a plugin's objects in, so the archive's other plugins (some GPL) stay out of the app.

    # nm on the ~300 MB archive takes 2-5 s, so its descriptor list is cached, keyed on the archive's path, the SDK
    # version and the archive's size and mtime; hashing the archive would cost as much as nm on every configure.
    file(SIZE "${GSTREAMER_XCFRAMEWORK_LIB}" _xcfw_lib_size)
    file(TIMESTAMP "${GSTREAMER_XCFRAMEWORK_LIB}" _xcfw_lib_mtime "%s")
    set(_xcfw_cache_key "${GSTREAMER_XCFRAMEWORK_LIB}|${GStreamer_VERSION}|${_xcfw_lib_size}|${_xcfw_lib_mtime}")
    set(_xcfw_cache_file "${CMAKE_BINARY_DIR}/qgc_xcfw_plugins.cmake")
    set(_xcfw_descs "")
    if(EXISTS "${_xcfw_cache_file}")
        include("${_xcfw_cache_file}")
        if(NOT "${_xcfw_cached_key}" STREQUAL "${_xcfw_cache_key}")
            set(_xcfw_descs "")
        endif()
    endif()
    if(NOT _xcfw_descs)
        # Prefer llvm-nm: it reads fat Mach-O natively, whereas Apple's BSD nm exits non-zero on a fat
        # binary unless given -arch. -gjU (global, names-only, defined) is portable across both.
        find_program(_xcfw_nm NAMES llvm-nm nm REQUIRED)

        # No -arch first (works for llvm-nm + thin archives); on failure (BSD nm on a fat binary) retry per slice.
        execute_process(
            COMMAND "${_xcfw_nm}" -gjU "${GSTREAMER_XCFRAMEWORK_LIB}"
            OUTPUT_VARIABLE _xcfw_nm_out
            ERROR_VARIABLE _xcfw_nm_err
            RESULT_VARIABLE _xcfw_nm_rc
        )
        if(NOT _xcfw_nm_rc EQUAL 0)
            foreach(_xcfw_arch IN LISTS CMAKE_OSX_ARCHITECTURES)
                execute_process(
                    COMMAND "${_xcfw_nm}" -arch "${_xcfw_arch}" -gjU "${GSTREAMER_XCFRAMEWORK_LIB}"
                    OUTPUT_VARIABLE _xcfw_nm_out
                    ERROR_VARIABLE _xcfw_nm_err
                    RESULT_VARIABLE _xcfw_nm_rc
                )
                if(_xcfw_nm_rc EQUAL 0)
                    break()
                endif()
            endforeach()
        endif()

        # Apple's nm (Xcode 26+) fails on objects it can't parse (the Rust gstaws bitcode) but still lists every other
        # object, so only an empty result is fatal.
        string(REGEX MATCHALL "_gst_plugin_[A-Za-z0-9_]+_get_desc" _xcfw_descs "${_xcfw_nm_out}")
        list(REMOVE_DUPLICATES _xcfw_descs)
        if(NOT _xcfw_descs)
            message(
                FATAL_ERROR
                    "nm produced no plugin descriptors for ${GSTREAMER_XCFRAMEWORK_LIB} (rc=${_xcfw_nm_rc})\n"
                    "  tool:        ${_xcfw_nm}\n"
                    "  archs tried: ${CMAKE_OSX_ARCHITECTURES} (plus a no-arch attempt)\n"
                    "  stderr:      ${_xcfw_nm_err}\n"
                    "If this is a fat Mach-O, install llvm (brew install llvm) so "
                    "llvm-nm is on PATH, or verify the framework slice arch matches "
                    "CMAKE_OSX_ARCHITECTURES."
            )
        elseif(NOT _xcfw_nm_rc EQUAL 0)
            message(
                STATUS "GStreamer iOS SDK: nm exited ${_xcfw_nm_rc} (unparseable objects, "
                       "e.g. Rust gstaws bitcode) but emitted usable symbols — continuing."
            )
        endif()
        file(
            WRITE "${_xcfw_cache_file}"
            "# Auto-generated cache of plugin descriptors enumerated from the iOS merged archive.\n"
            "# Regenerated when the archive, GStreamer_VERSION or the archive's size or mtime changes.\n"
            "set(_xcfw_cached_key \"${_xcfw_cache_key}\")\n" "set(_xcfw_descs \"${_xcfw_descs}\")\n"
        )
    endif()
    _qgc_gst_ios_select_static_plugins("${_xcfw_descs}" "${GSTREAMER_PLUGINS}" _xcfw_registered)
    list(LENGTH _xcfw_registered _xcfw_registered_count)
    list(LENGTH GSTREAMER_PLUGINS _xcfw_requested_count)
    list(LENGTH _xcfw_descs _xcfw_archived_count)
    message(
        STATUS "GStreamer iOS SDK: registering ${_xcfw_registered_count} of the ${_xcfw_requested_count} "
               "requested plugins (${_xcfw_archived_count} in the archive)"
    )
    set(GSTREAMER_STATIC_PLUGINS
        "${_xcfw_registered}"
        PARENT_SCOPE
    )

    # The component checks in Orchestrator.cmake look for GStreamer::<component>; here they are shims over the one
    # archive.
    foreach(_xcfw_comp IN LISTS GSTREAMER_SELECTED_COMPONENTS)
        if(NOT TARGET GStreamer::${_xcfw_comp})
            add_library(GStreamer::${_xcfw_comp} INTERFACE IMPORTED GLOBAL)
            target_link_libraries(GStreamer::${_xcfw_comp} INTERFACE GStreamer::GStreamer)
        endif()
    endforeach()

    _qgc_gst_ios_mobile_framework(${_xcfw_registered})
endfunction()
