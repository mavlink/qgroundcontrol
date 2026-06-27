# iOS GStreamer SDK discovery — invoked by Orchestrator.cmake.
# GStreamer 1.28 for iOS ships a universal GStreamer.framework (cerbero
# versioned bundle) whose Versions/1.0/GStreamer binary is a single merged
# static archive containing every library and plugin. Older/future SDKs may
# ship a GStreamer.xcframework with a fat libGStreamer.a per slice. Both reduce
# to the same single-merged-archive consumption model
# (_qgc_create_xcframework_targets) — discovery just locates the binary + Headers.

macro(_qgc_discover_ios_sdk)
    if(NOT CMAKE_HOST_APPLE)
        message(FATAL_ERROR "GStreamer for iOS can only be built on macOS")
    endif()

    if(GStreamer_FIND_VERSION VERSION_LESS "1.28.0")
        message(FATAL_ERROR
            "GStreamer for iOS requires version 1.28 or later. "
            "Got '${GStreamer_FIND_VERSION}' — bump gstreamer.version.ios in build-config.json.")
    endif()

    # Resolved below: _gst_ios_bundle (abs path to .xcframework or .framework)
    # and _gst_ios_bundle_kind ("xcframework" | "framework").
    set(_gst_ios_bundle "")
    set(_gst_ios_bundle_kind "")

    # ── System install ────────────────────────────────────────────────────────
    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(EXISTS "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.xcframework")
            set(_gst_ios_bundle "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.xcframework")
            set(_gst_ios_bundle_kind "xcframework")
        elseif(EXISTS "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
            set(_gst_ios_bundle "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
            set(_gst_ios_bundle_kind "framework")
        endif()
    endif()

    # ── Auto-download / expand ────────────────────────────────────────────────
    if(NOT _gst_ios_bundle)
        gstreamer_resolve_or_download_sdk(
            PLATFORM         ios
            CACHE_SUBDIR     "gstreamer-ios-${GStreamer_FIND_VERSION}"
            FILENAME_PRIMARY "gstreamer-ios.pkg"
            CACHE_DIR_OUT    _gst_ios_cache_dir
            ARCHIVE_OUT      _gst_ios_pkg
        )
        set(_gst_ios_expanded "${_gst_ios_cache_dir}/expanded")

        # Anchors that prove a complete expansion for either SDK layout.
        set(_gst_ios_anchor_globs
            "${_gst_ios_expanded}/*/GStreamer.xcframework/Info.plist"
            "${_gst_ios_expanded}/GStreamer.xcframework/Info.plist"
            "${_gst_ios_expanded}/*/GStreamer.framework/Versions/1.0/GStreamer"
            "${_gst_ios_expanded}/*/GStreamer.framework/GStreamer"
            "${_gst_ios_expanded}/*/GStreamer.framework/Versions/1.0/Headers/gst/gst.h"
            "${_gst_ios_expanded}/*/GStreamer.framework/Headers/gst/gst.h"
        )

        if(EXISTS "${_gst_ios_expanded}")
            set(_cached_anchor "")
            foreach(_glob IN LISTS _gst_ios_anchor_globs)
                file(GLOB_RECURSE _cached_anchor "${_glob}")
                if(_cached_anchor)
                    break()
                endif()
            endforeach()
            if(NOT _cached_anchor)
                message(STATUS "GStreamer: cached iOS expansion is incomplete; re-expanding")
                file(REMOVE_RECURSE "${_gst_ios_expanded}")
            endif()
        endif()

        if(NOT EXISTS "${_gst_ios_expanded}")
            _qgc_pkgutil_expand_and_validate("${_gst_ios_pkg}" "${_gst_ios_expanded}" "iOS")
        endif()

        # Prefer xcframework when present (future SDKs); fall back to framework.
        file(GLOB_RECURSE _xcfw_info_plists LIST_DIRECTORIES false
            "${_gst_ios_expanded}/*/GStreamer.xcframework/Info.plist"
            "${_gst_ios_expanded}/GStreamer.xcframework/Info.plist"
        )
        if(_xcfw_info_plists)
            list(GET _xcfw_info_plists 0 _xcfw_info_first)
            cmake_path(GET _xcfw_info_first PARENT_PATH _gst_ios_bundle)
            set(_gst_ios_bundle_kind "xcframework")
        else()
            file(GLOB_RECURSE _fw_dirs LIST_DIRECTORIES true
                "${_gst_ios_expanded}/*/GStreamer.framework"
                "${_gst_ios_expanded}/GStreamer.framework")
            foreach(_d IN LISTS _fw_dirs)
                if(IS_DIRECTORY "${_d}" AND _d MATCHES "/GStreamer\\.framework$")
                    set(_gst_ios_bundle "${_d}")
                    set(_gst_ios_bundle_kind "framework")
                    break()
                endif()
            endforeach()
        endif()

        if(NOT _gst_ios_bundle)
            file(GLOB _top_entries LIST_DIRECTORIES true "${_gst_ios_expanded}/*")
            file(GLOB_RECURSE _all_xcfw LIST_DIRECTORIES true "${_gst_ios_expanded}/*.xcframework")
            file(GLOB_RECURSE _all_fw   LIST_DIRECTORIES true "${_gst_ios_expanded}/*.framework")
            string(REPLACE ";" "\n  " _top_entries_str "${_top_entries}")
            string(REPLACE ";" "\n  " _all_xcfw_str "${_all_xcfw}")
            string(REPLACE ";" "\n  " _all_fw_str "${_all_fw}")
            message(FATAL_ERROR
                "Could not locate GStreamer.xcframework or GStreamer.framework in expanded iOS SDK at"
                " '${_gst_ios_expanded}'. The .pkg layout may have changed.\n"
                "Top-level entries:\n  ${_top_entries_str}\n"
                "All *.xcframework directories:\n  ${_all_xcfw_str}\n"
                "All *.framework directories:\n  ${_all_fw_str}")
        endif()
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    # ── Resolve merged static binary + Headers; force single-archive mode ─────
    # Both kinds consume one merged archive via _qgc_create_xcframework_targets.
    # Keep the three layout-mode flags mutually exclusive (Orchestrator validates).
    set(GStreamer_USE_XCFRAMEWORK ON)
    set(GStreamer_USE_FRAMEWORK OFF)
    set(GStreamer_USE_STATIC_LIBS OFF)

    if(_gst_ios_bundle_kind STREQUAL "xcframework")
        # SYSROOT may be a name ("iphonesimulator") or path ("…/iPhoneSimulator.sdk");
        # CMake regex has no case-insensitive flag, so fold case before matching.
        string(TOLOWER "${CMAKE_OSX_SYSROOT}" _gst_ios_sysroot_lc)
        if(_gst_ios_sysroot_lc MATCHES "simulator")
            file(GLOB _gst_ios_slice_dirs LIST_DIRECTORIES true "${_gst_ios_bundle}/ios-*-simulator")
        else()
            file(GLOB _gst_ios_slice_dirs LIST_DIRECTORIES true "${_gst_ios_bundle}/ios-*")
            list(FILTER _gst_ios_slice_dirs EXCLUDE REGEX "-(simulator|maccatalyst)$")
        endif()
        if(NOT _gst_ios_slice_dirs)
            message(FATAL_ERROR
                "GStreamer xcframework: no matching iOS slice found in ${_gst_ios_bundle}.\n"
                "Check ${_gst_ios_bundle}/Info.plist AvailableLibraries for the available slices.")
        endif()
        # Prefer a slice whose name carries the requested arch; fall back to first.
        set(_gst_ios_slice_dir "")
        if(CMAKE_OSX_ARCHITECTURES)
            foreach(_slice IN LISTS _gst_ios_slice_dirs)
                foreach(_arch IN LISTS CMAKE_OSX_ARCHITECTURES)
                    if(_slice MATCHES "(^|[-_/])${_arch}([-_]|$)")
                        set(_gst_ios_slice_dir "${_slice}")
                        break()
                    endif()
                endforeach()
                if(_gst_ios_slice_dir)
                    break()
                endif()
            endforeach()
        endif()
        if(NOT _gst_ios_slice_dir)
            list(GET _gst_ios_slice_dirs 0 _gst_ios_slice_dir)
        endif()
        if(NOT EXISTS "${_gst_ios_slice_dir}")
            message(FATAL_ERROR
                "GStreamer xcframework slice '${_gst_ios_slice_dir}' is not a directory.")
        endif()
        set(GStreamer_ROOT_DIR        "${_gst_ios_slice_dir}")
        set(GSTREAMER_XCFRAMEWORK_LIB "${_gst_ios_slice_dir}/libGStreamer.a")
        set(_gst_ios_include          "${_gst_ios_slice_dir}/Headers")
        set(_gst_ios_layout_root      "${_gst_ios_slice_dir}")
        set(_gst_ios_xcfw_bundle      "${_gst_ios_bundle}")
    else()
        # cerbero universal .framework: one merged fat binary covers all arches.
        # Versioned bundle (Versions/1.0/...) is canonical; tolerate a flat bundle.
        if(EXISTS "${_gst_ios_bundle}/Versions/1.0/GStreamer")
            set(_gst_ios_layout_root "${_gst_ios_bundle}/Versions/1.0")
        elseif(EXISTS "${_gst_ios_bundle}/GStreamer")
            set(_gst_ios_layout_root "${_gst_ios_bundle}")
        else()
            message(FATAL_ERROR
                "GStreamer.framework at ${_gst_ios_bundle} has no merged binary at "
                "Versions/1.0/GStreamer or GStreamer. The SDK layout may have changed.")
        endif()
        set(GStreamer_ROOT_DIR        "${_gst_ios_layout_root}")
        set(GSTREAMER_XCFRAMEWORK_LIB "${_gst_ios_layout_root}/GStreamer")
        # Headers may live under Versions/1.0/Headers or be a symlink at the root.
        if(EXISTS "${_gst_ios_layout_root}/Headers")
            set(_gst_ios_include "${_gst_ios_layout_root}/Headers")
        elseif(EXISTS "${_gst_ios_bundle}/Headers")
            set(_gst_ios_include "${_gst_ios_bundle}/Headers")
        else()
            message(FATAL_ERROR
                "GStreamer.framework at ${_gst_ios_bundle} has no Headers/ directory.")
        endif()
        set(_gst_ios_xcfw_bundle     "${_gst_ios_bundle}")
    endif()

    if(NOT EXISTS "${GSTREAMER_XCFRAMEWORK_LIB}")
        message(FATAL_ERROR
            "GStreamer iOS merged binary not found at ${GSTREAMER_XCFRAMEWORK_LIB}")
    endif()

    # Resolve Info.plist so the real CFBundleShortVersionString can re-check the
    # floor — the check above only sees the requested version, missing downgrades.
    set(GStreamer_IOS_INFO_PLIST "")
    if(_gst_ios_bundle_kind STREQUAL "xcframework")
        # Each slice carries a per-arch GStreamer.framework with its own Info.plist.
        foreach(_pl
            "${_gst_ios_slice_dir}/GStreamer.framework/Info.plist"
            "${_gst_ios_slice_dir}/GStreamer.framework/Resources/Info.plist")
            if(EXISTS "${_pl}")
                set(GStreamer_IOS_INFO_PLIST "${_pl}")
                break()
            endif()
        endforeach()
    else()
        # cerbero universal .framework: versioned Resources/Info.plist is canonical.
        foreach(_pl
            "${_gst_ios_layout_root}/Resources/Info.plist"
            "${_gst_ios_bundle}/Resources/Info.plist"
            "${_gst_ios_bundle}/Info.plist")
            if(EXISTS "${_pl}")
                set(GStreamer_IOS_INFO_PLIST "${_pl}")
                break()
            endif()
        endforeach()
    endif()

    # gstreamer_create_layout_target sets GSTREAMER_LIB/PLUGIN/INCLUDE/XCFRAMEWORK_PATH.
    # TYPE XCFRAMEWORK collapses lib/plugin paths to the layout root (no lib/ subdir
    # in a merged binary). XCFRAMEWORK_BUNDLE = the .xcframework or .framework dir;
    # Layout.cmake only EXISTS-checks SDK_ROOT and the bundle, both real dirs here.
    gstreamer_create_layout_target(
        SDK_ROOT           "${_gst_ios_layout_root}"
        TYPE               XCFRAMEWORK
        INCLUDE_PATH       "${_gst_ios_include}"
        XCFRAMEWORK_BUNDLE "${_gst_ios_xcfw_bundle}"
    )

    # iOS SDK ships no CA bundle reachable from libgioopenssl; fetch Mozilla NSS.
    _qgc_download_ios_ca_bundle()
endmacro()

# Downloads ca-certificates.crt to the iOS SDK cache and sets
# GStreamer_IOS_CA_BUNDLE (cache var) to its absolute path. Re-uses the same
# CPM_SOURCE_CACHE root the SDK download uses so a `rm -rf build` doesn't
# re-fetch it.
function(_qgc_download_ios_ca_bundle)
    if(CPM_SOURCE_CACHE)
        set(_ca_dir "${CPM_SOURCE_CACHE}/gstreamer-ios-ca")
    else()
        set(_ca_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-ios-ca")
    endif()

    set(_ca_hash_arg "")
    if(QGC_BUILD_CONFIG_CONTENT)
        string(JSON _ca_sha256 ERROR_VARIABLE _ca_err
            GET "${QGC_BUILD_CONFIG_CONTENT}" "gstreamer" "ca_bundle_sha256")
        if(_ca_sha256 AND NOT _ca_err)
            set(_ca_hash_arg EXPECTED_HASH "SHA256=${_ca_sha256}")
        endif()
    endif()
    if(NOT _ca_hash_arg)
        # This bundle becomes the app's TLS trust root; an unpinned fetch lets a
        # MITM inject one. Pinning is default — opt out via QGC_GST_ALLOW_UNVERIFIED_CA.
        if(NOT QGC_GST_ALLOW_UNVERIFIED_CA)
            message(FATAL_ERROR
                "GStreamer: gstreamer.ca_bundle_sha256 is missing from build-config.json, so the "
                "iOS CA bundle (the app's TLS trust root) cannot be verified. Pin it from "
                "https://curl.se/ca/cacert.pem.sha256 and commit it to build-config.json. "
                "To fetch the bundle UNVERIFIED anyway (NOT for CI/release builds), set "
                "-DQGC_GST_ALLOW_UNVERIFIED_CA=ON.")
        endif()
        message(WARNING "GStreamer: gstreamer.ca_bundle_sha256 missing from build-config.json and "
            "QGC_GST_ALLOW_UNVERIFIED_CA is set; iOS CA bundle will be fetched UNVERIFIED. "
            "Pin it from https://curl.se/ca/cacert.pem.sha256")
    endif()

    qgc_resilient_download(
        FILENAME        ca-certificates.crt
        DESTINATION_DIR "${_ca_dir}"
        URLS            "https://curl.se/ca/cacert.pem"
        RESULT_VAR      _ca_path
        LOG_TAG         "iOS CA bundle"
        FAILURE_HINT    "Network is required at first iOS configure to fetch the Mozilla CA bundle from curl.se."
        ${_ca_hash_arg}
    )

    set(GStreamer_IOS_CA_BUNDLE "${_ca_path}" CACHE FILEPATH
        "Mozilla CA bundle bundled into iOS app resources at ssl/certs/ca-certificates.crt" FORCE)
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# _qgc_create_xcframework_targets
# Build IMPORTED targets directly from the iOS xcframework's fat .a — bypasses
# pkg-config entirely. Caller must have set GSTREAMER_XCFRAMEWORK_LIB and
# GSTREAMER_INCLUDE_PATH (done by _qgc_discover_ios_sdk above).
# Function: exports GStreamer_FOUND / GStreamer_VERSION via PARENT_SCOPE and creates GLOBAL IMPORTED targets.
# ─────────────────────────────────────────────────────────────────────────────
function(_qgc_create_xcframework_targets)
    # Read the bundle's real version from Info.plist to re-check the floor — the
    # _discover check only sees the requested version. Warn + fall back if unreadable.
    set(_xcfw_bundle_version "")
    if(GStreamer_IOS_INFO_PLIST AND EXISTS "${GStreamer_IOS_INFO_PLIST}")
        # PlistBuddy (always present on macOS hosts) reads binary or XML plists;
        # fall back to a regex over the XML form if it is ever unavailable.
        find_program(_xcfw_plistbuddy NAMES PlistBuddy
            PATHS /usr/libexec NO_DEFAULT_PATH NO_CACHE)
        if(_xcfw_plistbuddy)
            execute_process(
                COMMAND "${_xcfw_plistbuddy}" -c "Print :CFBundleShortVersionString"
                        "${GStreamer_IOS_INFO_PLIST}"
                OUTPUT_VARIABLE _xcfw_bundle_version
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
                RESULT_VARIABLE _xcfw_plist_rc)
            if(NOT _xcfw_plist_rc EQUAL 0)
                set(_xcfw_bundle_version "")
            endif()
        endif()
        if(NOT _xcfw_bundle_version)
            file(READ "${GStreamer_IOS_INFO_PLIST}" _xcfw_plist_xml)
            string(REGEX MATCH
                "<key>CFBundleShortVersionString</key>[ \t\r\n]*<string>([^<]+)</string>"
                _xcfw_plist_match "${_xcfw_plist_xml}")
            if(_xcfw_plist_match)
                set(_xcfw_bundle_version "${CMAKE_MATCH_1}")
            endif()
        endif()
    endif()

    if(_xcfw_bundle_version)
        if(_xcfw_bundle_version VERSION_LESS "1.28.0")
            message(FATAL_ERROR
                "GStreamer iOS bundle reports version '${_xcfw_bundle_version}' "
                "(from ${GStreamer_IOS_INFO_PLIST}), which is older than the required "
                "1.28 floor. A downgraded or stale GStreamer.framework was found — "
                "remove it / clear the SDK cache and re-fetch the pinned version.")
        endif()
        if(GStreamer_FIND_VERSION AND _xcfw_bundle_version VERSION_LESS GStreamer_FIND_VERSION)
            message(WARNING
                "GStreamer iOS bundle version '${_xcfw_bundle_version}' is older than the "
                "requested '${GStreamer_FIND_VERSION}'. Using the bundle's actual version.")
        endif()
        set(GStreamer_VERSION "${_xcfw_bundle_version}" PARENT_SCOPE)
        set(GStreamer_VERSION "${_xcfw_bundle_version}")
    elseif(NOT GStreamer_VERSION)
        message(WARNING
            "GStreamer iOS: could not read CFBundleShortVersionString from the bundle "
            "Info.plist (looked at '${GStreamer_IOS_INFO_PLIST}'). Falling back to the "
            "REQUESTED version '${GStreamer_FIND_VERSION}' — the bundle's actual version "
            "is UNVERIFIED, so a downgraded SDK cannot be detected.")
        set(GStreamer_VERSION "${GStreamer_FIND_VERSION}" PARENT_SCOPE)
        set(GStreamer_VERSION "${GStreamer_FIND_VERSION}")
    endif()

    # ── xcframework path: create IMPORTED targets directly from the fat .a ───
    # No pkg-config or .framework; all APIs and plugins live in one archive.
    if(NOT TARGET GStreamer::GStreamer)
        add_library(GStreamer_static STATIC IMPORTED GLOBAL)
        set_target_properties(GStreamer_static PROPERTIES
            IMPORTED_LOCATION "${GSTREAMER_XCFRAMEWORK_LIB}"
        )
        add_library(GStreamer::GStreamer INTERFACE IMPORTED GLOBAL)
        target_link_libraries(GStreamer::GStreamer INTERFACE
            GStreamer_static
        )
        # iOS .framework has flat Headers/; macOS-style installs have the gstreamer-1.0 /
        # glib-2.0 subdirs. Add only what exists — INTERFACE_INCLUDE_DIRECTORIES is validated.
        target_include_directories(GStreamer::GStreamer INTERFACE "${GSTREAMER_INCLUDE_PATH}")
        foreach(_inc_sub IN ITEMS gstreamer-1.0 glib-2.0)
            if(IS_DIRECTORY "${GSTREAMER_INCLUDE_PATH}/${_inc_sub}")
                target_include_directories(GStreamer::GStreamer INTERFACE "${GSTREAMER_INCLUDE_PATH}/${_inc_sub}")
            endif()
        endforeach()
        # System frameworks required by GStreamer on iOS.
        # Note: gstreamer-ios 1.28+ bundles libass (CoreText), MoltenVK (Metal/IOSurface/QuartzCore),
        # applemedia iosassetsrc (AssetsLibrary), and EAGL/CAMetalLayer (QuartzCore) — all of which
        # demand additional system frameworks at the *consumer* link step.
        set(_xcfw_required_frameworks
            Foundation AVFoundation AudioToolbox VideoToolbox CoreMedia CoreVideo
            CoreAudio CoreGraphics Security UIKit CoreFoundation CoreText
            IOSurface Metal QuartzCore
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
        target_link_libraries(GStreamer::GStreamer INTERFACE
            ${_xcfw_resolved_libs}
            "-lresolv" "-liconv" "-lz" "-lbz2"
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
        # gstreamer-ios 1.28 bundles many Rust-built static libs (gst-plugins-rs); each contributes
        # a `_rust_eh_personality` reference and Mach-O compact unwind can encode only ~4 unique
        # personalities. Switch to DWARF unwind tables to sidestep the limit. Slightly larger
        # binary; runtime perf impact is negligible (only used during exception unwinding, and
        # GStreamer doesn't throw C++ exceptions across its API surface).
        target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-no_compact_unwind")
        target_compile_definitions(GStreamer::GStreamer INTERFACE
            QGC_GST_STATIC_BUILD
        )
        # Keep the find_library cache entries — re-resolving 17 system frameworks
        # on every reconfigure is the dominant configure-time cost on iOS.
    endif()

    # Per-API alias targets — load-bearing for the optional-component FOUND
    # check in Orchestrator.cmake (`if(TARGET GStreamer::${_api})`). On the
    # pkg-config flow, FindGStreamer creates these targets directly; on the
    # xcframework flow the entire library lives in one .a, so we synthesize
    # empty INTERFACE shims that link back to GStreamer::GStreamer. Iterating
    # GSTREAMER_APIS keeps a future find_package(... COMPONENTS NewOne) call
    # working without editing this file.
    foreach(_xcfw_comp IN LISTS GSTREAMER_APIS GSTREAMER_PLUGINS)
        if(NOT TARGET GStreamer::${_xcfw_comp})
            add_library(GStreamer::${_xcfw_comp} INTERFACE IMPORTED GLOBAL)
            target_link_libraries(GStreamer::${_xcfw_comp} INTERFACE GStreamer::GStreamer)
        endif()
        set(GStreamer_${_xcfw_comp}_FOUND TRUE PARENT_SCOPE)
    endforeach()

    # Build the xcframework mobile init shim — calls gst_init_static_plugins().
    if(NOT TARGET GStreamer::mobile)
        enable_language(OBJC OBJCXX)

        # GStreamer 1.28+ ships every plugin compiled into libGStreamer.a but
        # provides no auto-registration entrypoint; enumerate plugin descriptors
        # from the archive and emit explicit GST_PLUGIN_STATIC_REGISTER() calls.
        # Skip list (plugins whose static deps aren't in the slice) is owned by
        # GStreamerPluginPolicy.
        gstreamer_xcfw_skip(_xcfw_skip_plugins)

        # Cache the nm-extracted plugin descriptor list — running nm -gjU on the
        # ~300MB libGStreamer.a costs 2-5s per reconfigure. Key on archive
        # mtime+size and SDK version so a swapped xcframework re-invokes nm.
        file(SIZE "${GSTREAMER_XCFRAMEWORK_LIB}" _xcfw_lib_size)
        file(TIMESTAMP "${GSTREAMER_XCFRAMEWORK_LIB}" _xcfw_lib_mtime "%s")
        # Hash the archive so a same-size, mtime-normalized rebuild (CI cache restore)
        # still re-invokes nm instead of reusing a stale plugin list.
        file(SHA256 "${GSTREAMER_XCFRAMEWORK_LIB}" _xcfw_lib_hash)
        set(_xcfw_cache_key "${GStreamer_VERSION}|${_xcfw_lib_size}|${_xcfw_lib_mtime}|${_xcfw_lib_hash}|${_xcfw_skip_plugins}")
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

            # Apple's nm (Xcode 26+) exits non-zero on objects it can't parse (e.g. the
            # Rust gstaws bitcode: "Unknown attribute kind") but still prints symbols for
            # every object it can read. Extract from whatever it produced; fail only if
            # that yielded no descriptors, not on a non-zero exit alone.
            string(REGEX MATCHALL "_gst_plugin_[A-Za-z0-9_]+_get_desc" _xcfw_descs "${_xcfw_nm_out}")
            list(REMOVE_DUPLICATES _xcfw_descs)
            if(NOT _xcfw_descs)
                message(FATAL_ERROR
                    "nm produced no plugin descriptors for ${GSTREAMER_XCFRAMEWORK_LIB} (rc=${_xcfw_nm_rc})\n"
                    "  tool:        ${_xcfw_nm}\n"
                    "  archs tried: ${CMAKE_OSX_ARCHITECTURES} (plus a no-arch attempt)\n"
                    "  stderr:      ${_xcfw_nm_err}\n"
                    "If this is a fat Mach-O, install llvm (brew install llvm) so "
                    "llvm-nm is on PATH, or verify the framework slice arch matches "
                    "CMAKE_OSX_ARCHITECTURES.")
            elseif(NOT _xcfw_nm_rc EQUAL 0)
                message(STATUS
                    "GStreamer xcframework: nm exited ${_xcfw_nm_rc} (unparseable objects, "
                    "e.g. Rust gstaws bitcode) but emitted usable symbols — continuing.")
            endif()
            file(WRITE "${_xcfw_cache_file}"
                "# Auto-generated cache of plugin descriptors enumerated from the iOS xcframework.\n"
                "# Regenerated when GStreamer_VERSION or libGStreamer.a (size+mtime) changes.\n"
                "set(_xcfw_cached_key \"${_xcfw_cache_key}\")\n"
                "set(_xcfw_descs \"${_xcfw_descs}\")\n"
            )
        endif()
        set(_xcfw_names "")
        foreach(_sym IN LISTS _xcfw_descs)
            string(REGEX REPLACE "^_gst_plugin_(.+)_get_desc$" "\\1" _name "${_sym}")
            if(_name IN_LIST _xcfw_skip_plugins)
                continue()
            endif()
            list(APPEND _xcfw_names "${_name}")
        endforeach()
        _gst_emit_static_plugin_registration(_xcfw_names _xcfw_decl _xcfw_reg)
        list(LENGTH _xcfw_names _xcfw_used)
        list(LENGTH _xcfw_descs _xcfw_n)
        message(STATUS "GStreamer xcframework: registering ${_xcfw_used}/${_xcfw_n} static plugins (skipped: ${_xcfw_skip_plugins})")
        # Match the substitution variable names _qgc_create_android_mobile_target populates so the
        # template at GStreamer/gst_static_plugins.c.in works for both call sites.
        set(PLUGINS_DECLARATION  "${_xcfw_decl}")
        set(PLUGINS_REGISTRATION "${_xcfw_reg}")
        # gioopenssl is statically linked into libGStreamer.a — load it so it
        # registers as the default GIO TLS backend (verified via nm: symbol
        # _g_io_openssl_load is exported from the xcframework slice).
        set(G_IO_MODULES_DECLARE "GST_G_IO_MODULE_DECLARE(openssl);")
        set(G_IO_MODULES_LOAD    "    GST_G_IO_MODULE_LOAD(openssl);")

        set(_xcfw_static_shim "${CMAKE_BINARY_DIR}/${GStreamer_Mobile_MODULE_NAME}_static.c")
        # Template lives in src/VideoManager/VideoReceiver/GStreamer/ alongside
        # the C++ code that calls gst_init_static_plugins().
        set(_qgc_gst_src "${CMAKE_SOURCE_DIR}/src/VideoManager/VideoReceiver/GStreamer")
        configure_file(
            "${_qgc_gst_src}/gst_static_plugins.c.in"
            "${_xcfw_static_shim}"
            @ONLY
        )
        add_library(GStreamerMobileXcfw SHARED)
        target_sources(GStreamerMobileXcfw PRIVATE "${_xcfw_static_shim}")
        set_source_files_properties("${_xcfw_static_shim}" PROPERTIES GENERATED TRUE)
        target_link_libraries(GStreamerMobileXcfw PRIVATE GStreamer::GStreamer)
        set_target_properties(GStreamerMobileXcfw PROPERTIES
            LIBRARY_OUTPUT_NAME ${GStreamer_Mobile_MODULE_NAME}
            FRAMEWORK TRUE
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.gstreamer.GStreamerMobile
            LINKER_LANGUAGE CXX
        )
        add_library(GStreamer::mobile ALIAS GStreamerMobileXcfw)
        add_library(GStreamerMobile ALIAS GStreamerMobileXcfw)
        set(GStreamerMobile_FOUND TRUE PARENT_SCOPE)
        set(GStreamerMobile_mobile_FOUND TRUE PARENT_SCOPE)
    endif()

    set(GStreamer_FOUND TRUE PARENT_SCOPE)
endfunction()
