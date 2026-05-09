# iOS GStreamer SDK (xcframework) discovery — invoked by Orchestrator.cmake.

macro(_qgc_discover_ios_sdk)
    if(NOT CMAKE_HOST_APPLE)
        message(FATAL_ERROR "GStreamer for iOS can only be built on macOS")
    endif()

    # iOS requires the xcframework SDK layout introduced in 1.28.
    if(GStreamer_FIND_VERSION VERSION_LESS "1.28.0")
        message(FATAL_ERROR
            "GStreamer for iOS requires version 1.28 or later (xcframework SDK layout). "
            "Got '${GStreamer_FIND_VERSION}' — bump gstreamer.version.ios in build-config.json.")
    endif()

    # ── System install ────────────────────────────────────────────────────────
    if(NOT DEFINED GStreamer_ROOT_DIR
       AND EXISTS "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.xcframework")
        set(_gst_ios_system_xcfw "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.xcframework")
    endif()

    # ── Auto-download / expand ────────────────────────────────────────────────
    if(NOT DEFINED _gst_ios_system_xcfw)
        gstreamer_resolve_or_download_sdk(
            PLATFORM         ios
            CACHE_SUBDIR     "gstreamer-ios-${GStreamer_FIND_VERSION}"
            FILENAME_PRIMARY "gstreamer-ios.pkg"
            CACHE_DIR_OUT    _gst_ios_cache_dir
            ARCHIVE_OUT      _gst_ios_pkg
        )
        set(_gst_ios_expanded "${_gst_ios_cache_dir}/expanded")

        if(EXISTS "${_gst_ios_expanded}")
            file(GLOB_RECURSE _cached_anchor
                "${_gst_ios_expanded}/*/GStreamer.xcframework/Info.plist"
                "${_gst_ios_expanded}/GStreamer.xcframework/Info.plist")
            if(NOT _cached_anchor)
                message(STATUS "GStreamer: cached iOS expansion is incomplete; re-expanding")
                file(REMOVE_RECURSE "${_gst_ios_expanded}")
            endif()
        endif()

        if(NOT EXISTS "${_gst_ios_expanded}")
            message(STATUS "Expanding GStreamer iOS package...")
            execute_process(
                COMMAND pkgutil --expand-full "${_gst_ios_pkg}" "${_gst_ios_expanded}"
                RESULT_VARIABLE _pkgutil_rc
            )
            if(NOT _pkgutil_rc EQUAL 0)
                file(REMOVE_RECURSE "${_gst_ios_expanded}")
                message(FATAL_ERROR
                    "pkgutil failed to expand GStreamer iOS .pkg (exit code: ${_pkgutil_rc})")
            endif()
            _qgc_validate_expanded_pkg("${_gst_ios_expanded}" "iOS")
        endif()

        # Locate xcframework. Try common shallow nesting first; fall back to a recursive walk.
        file(GLOB_RECURSE _xcfw_info_plists LIST_DIRECTORIES false
            "${_gst_ios_expanded}/*/GStreamer.xcframework/Info.plist"
            "${_gst_ios_expanded}/GStreamer.xcframework/Info.plist"
        )
        if(NOT _xcfw_info_plists)
            file(GLOB_RECURSE _all_dirs LIST_DIRECTORIES true "${_gst_ios_expanded}/*")
            foreach(_d IN LISTS _all_dirs)
                if(IS_DIRECTORY "${_d}" AND _d MATCHES "/GStreamer\\.xcframework$"
                   AND EXISTS "${_d}/Info.plist")
                    list(APPEND _xcfw_info_plists "${_d}/Info.plist")
                    break()
                endif()
            endforeach()
        endif()

        if(NOT _xcfw_info_plists)
            file(GLOB _top_entries LIST_DIRECTORIES true "${_gst_ios_expanded}/*")
            file(GLOB_RECURSE _all_xcframeworks LIST_DIRECTORIES true
                "${_gst_ios_expanded}/*.xcframework")
            string(REPLACE ";" "\n  " _top_entries_str "${_top_entries}")
            string(REPLACE ";" "\n  " _all_xcframeworks_str "${_all_xcframeworks}")
            message(FATAL_ERROR
                "Could not locate GStreamer.xcframework in expanded iOS SDK at"
                " '${_gst_ios_expanded}'. The .pkg layout may have changed.\n"
                "Top-level entries:\n  ${_top_entries_str}\n"
                "All *.xcframework directories:\n  ${_all_xcframeworks_str}")
        endif()

        list(GET _xcfw_info_plists 0 _xcfw_info_first)
        cmake_path(GET _xcfw_info_first PARENT_PATH _gst_ios_system_xcfw)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    # ── Pick slice for current sysroot ────────────────────────────────────────
    # xcframework and framework modes are mutually exclusive; xcfw confirmed above.
    set(GStreamer_USE_XCFRAMEWORK ON)
    set(GStreamer_USE_FRAMEWORK OFF)
    if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
        set(_xcfw_slice "ios-arm64_x86_64-simulator")
    else()
        set(_xcfw_slice "ios-arm64")
    endif()

    set(_xcfw_slice_dir "${_gst_ios_system_xcfw}/${_xcfw_slice}")
    if(NOT EXISTS "${_xcfw_slice_dir}")
        message(FATAL_ERROR
            "GStreamer xcframework slice '${_xcfw_slice}' not found in ${_gst_ios_system_xcfw}.\n"
            "Check ${_gst_ios_system_xcfw}/Info.plist AvailableLibraries for the available slices.")
    endif()

    # All code lives in libGStreamer.a; xcframework has no lib/ or lib/gstreamer-1.0/.
    set(GStreamer_ROOT_DIR        "${_xcfw_slice_dir}")
    set(GSTREAMER_XCFRAMEWORK_LIB "${_xcfw_slice_dir}/libGStreamer.a")
    # gstreamer_create_layout_target sets GSTREAMER_LIB/PLUGIN/INCLUDE/XCFRAMEWORK_PATH.
    gstreamer_create_layout_target(
        SDK_ROOT           "${_xcfw_slice_dir}"
        TYPE               XCFRAMEWORK
        INCLUDE_PATH       "${_xcfw_slice_dir}/Headers"
        XCFRAMEWORK_BUNDLE "${_gst_ios_system_xcfw}"
    )

    # iOS xcframework ships no CA bundle and Apple's keychain isn't reachable
    # from libgioopenssl, so fetch a Mozilla NSS extract from curl.se. Cached
    # alongside the SDK in CPM_SOURCE_CACHE; refreshed on clean rebuilds only.
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

    qgc_resilient_download(
        FILENAME        ca-certificates.crt
        DESTINATION_DIR "${_ca_dir}"
        URLS            "https://curl.se/ca/cacert.pem"
        RESULT_VAR      _ca_path
        LOG_TAG         "iOS CA bundle"
        FAILURE_HINT    "Network is required at first iOS configure to fetch the Mozilla CA bundle from curl.se."
    )

    set(GStreamer_IOS_CA_BUNDLE "${_ca_path}" CACHE FILEPATH
        "Mozilla CA bundle bundled into iOS app resources at ssl/certs/ca-certificates.crt" FORCE)
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# _qgc_create_xcframework_targets
# Build IMPORTED targets directly from the iOS xcframework's fat .a — bypasses
# pkg-config entirely. Caller must have set GSTREAMER_XCFRAMEWORK_LIB and
# GSTREAMER_INCLUDE_PATH (done by _qgc_discover_ios_sdk above).
# Macro (not function): GStreamer_FOUND / GStreamer_VERSION / target globals
# must propagate to caller scope.
# ─────────────────────────────────────────────────────────────────────────────
function(_qgc_create_xcframework_targets)
    # No pkg-config find_package runs on this path — populate GStreamer_VERSION
    # from the requested version so cache keys / status messages have a value.
    if(NOT GStreamer_VERSION)
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
            CoreAudio CoreGraphics Security OpenGLES UIKit CoreFoundation CoreText
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
        find_library(_xcfw_assetslibrary AssetsLibrary)
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
    foreach(_xcfw_comp IN LISTS GSTREAMER_APIS)
        if(NOT TARGET GStreamer::${_xcfw_comp})
            add_library(GStreamer::${_xcfw_comp} INTERFACE IMPORTED GLOBAL)
            target_link_libraries(GStreamer::${_xcfw_comp} INTERFACE GStreamer::GStreamer)
        endif()
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
        set(_xcfw_cache_key "${GStreamer_VERSION}|${_xcfw_lib_size}|${_xcfw_lib_mtime}|${_xcfw_skip_plugins}")
        set(_xcfw_cache_file "${CMAKE_BINARY_DIR}/qgc_xcfw_plugins.cmake")
        set(_xcfw_descs "")
        if(EXISTS "${_xcfw_cache_file}")
            include("${_xcfw_cache_file}")
            if(NOT "${_xcfw_cached_key}" STREQUAL "${_xcfw_cache_key}")
                set(_xcfw_descs "")
            endif()
        endif()
        if(NOT _xcfw_descs)
            find_program(_xcfw_nm NAMES nm llvm-nm REQUIRED)
            execute_process(
                COMMAND "${_xcfw_nm}" -gjU "${GSTREAMER_XCFRAMEWORK_LIB}"
                OUTPUT_VARIABLE _xcfw_nm_out
                ERROR_QUIET
                RESULT_VARIABLE _xcfw_nm_rc
            )
            if(NOT _xcfw_nm_rc EQUAL 0)
                message(FATAL_ERROR "nm failed on ${GSTREAMER_XCFRAMEWORK_LIB} (rc=${_xcfw_nm_rc})")
            endif()
            string(REGEX MATCHALL "_gst_plugin_[A-Za-z0-9_]+_get_desc" _xcfw_descs "${_xcfw_nm_out}")
            list(REMOVE_DUPLICATES _xcfw_descs)
            file(WRITE "${_xcfw_cache_file}"
                "# Auto-generated cache of plugin descriptors enumerated from the iOS xcframework.\n"
                "# Regenerated when GStreamer_VERSION or libGStreamer.a (size+mtime) changes.\n"
                "set(_xcfw_cached_key \"${_xcfw_cache_key}\")\n"
                "set(_xcfw_descs \"${_xcfw_descs}\")\n"
            )
        endif()
        set(_xcfw_decl "")
        set(_xcfw_reg  "")
        set(_xcfw_used 0)
        foreach(_sym IN LISTS _xcfw_descs)
            string(REGEX REPLACE "^_gst_plugin_(.+)_get_desc$" "\\1" _name "${_sym}")
            if(_name IN_LIST _xcfw_skip_plugins)
                continue()
            endif()
            string(APPEND _xcfw_decl "GST_PLUGIN_STATIC_DECLARE(${_name});\n")
            string(APPEND _xcfw_reg  "    QGC_REGISTER_STATIC_PLUGIN(${_name});\n")
            math(EXPR _xcfw_used "${_xcfw_used} + 1")
        endforeach()
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
        )
        add_library(GStreamer::mobile ALIAS GStreamerMobileXcfw)
        add_library(GStreamerMobile ALIAS GStreamerMobileXcfw)
        set(GStreamerMobile_FOUND TRUE PARENT_SCOPE)
        set(GStreamerMobile_mobile_FOUND TRUE PARENT_SCOPE)
    endif()

    set(GStreamer_FOUND TRUE PARENT_SCOPE)
endfunction()
