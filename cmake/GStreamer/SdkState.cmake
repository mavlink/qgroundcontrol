# GStreamer SDK state carried across configures. GStreamer_ROOT_DIR is user input only; the
# SDK discovery picked is recorded in the INTERNAL QGC_GST_RESOLVED_{ROOT,VERSION,AUTO} entries
# (also read from CMakeCache.txt by .github/scripts), and cached probe results are tied to an SDK
# fingerprint so they are recomputed whenever the SDK changes.

include_guard(GLOBAL)

# _qgc_gst_sdk_pc_version(<lib_dir> <out_var>)
# Returns the Version: field of <lib_dir>/pkgconfig/gstreamer-1.0.pc, or "" when the SDK has none.
function(_qgc_gst_sdk_pc_version lib_dir out_var)
    set(_pc_file "${lib_dir}/pkgconfig/gstreamer-1.0.pc")
    set(_version "")
    if(EXISTS "${_pc_file}")
        file(
            STRINGS "${_pc_file}" _version_line
            REGEX "^Version:[ \t]*"
            LIMIT_COUNT 1
        )
        string(REGEX REPLACE "^Version:[ \t]*" "" _version "${_version_line}")
        string(STRIP "${_version}" _version)
    endif()
    set(${out_var}
        "${_version}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_select_sdk_root(<out_root> <out_auto> <user_root> <requested_version>)
# Chooses the root platform discovery starts from: the user's root, else the previously
# auto-downloaded root while it still exists and was fetched for <requested_version>.
# An empty <out_root> means discovery must locate or download the SDK again.
function(_qgc_gst_select_sdk_root out_root out_auto user_root requested_version)
    set(_root "")
    set(_auto FALSE)
    if(user_root)
        set(_root "${user_root}")
    elseif(
        "$CACHE{QGC_GST_RESOLVED_AUTO}"
        AND "$CACHE{QGC_GST_RESOLVED_VERSION}" STREQUAL "${requested_version}"
        AND IS_DIRECTORY "$CACHE{QGC_GST_RESOLVED_ROOT}"
    )
        set(_root "$CACHE{QGC_GST_RESOLVED_ROOT}")
        set(_auto TRUE)
    endif()
    set(${out_root}
        "${_root}"
        PARENT_SCOPE
    )
    set(${out_auto}
        "${_auto}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_record_resolved_sdk(<root> <requested_version> <auto_downloaded>)
function(_qgc_gst_record_resolved_sdk root requested_version auto_downloaded)
    # CI tooling reads these names from CMakeCache.txt, so they keep the public QGC_ prefix.
    # cmake-lint: disable=C0103
    set(QGC_GST_RESOLVED_ROOT
        "${root}"
        CACHE INTERNAL "GStreamer SDK root selected by discovery"
    )
    set(QGC_GST_RESOLVED_VERSION
        "${requested_version}"
        CACHE INTERNAL "GStreamer version the resolved SDK was selected for"
    )
    set(QGC_GST_RESOLVED_AUTO
        "${auto_downloaded}"
        CACHE INTERNAL "Whether the resolved GStreamer SDK was auto-downloaded"
    )
endfunction()

# _qgc_gst_refresh_sdk_cache(<fingerprint>)
# pkg-config results, find_library/find_program paths and try-compile probes are cached under
# fixed names and never re-run on their own. When <fingerprint> (the SDK identity) differs from
# the one they were computed for, drop them so this configure recomputes them.
function(_qgc_gst_refresh_sdk_cache fingerprint)
    if(DEFINED CACHE{_QGC_GST_SDK_FINGERPRINT} AND "$CACHE{_QGC_GST_SDK_FINGERPRINT}" STREQUAL "${fingerprint}")
        return()
    endif()
    set(_stale_regex
        "^(__pkg_config_(checked|arguments)_PC_GStreamer|PC_GStreamer|pkgcfg_lib_PC_GStreamer|_gst_|GStreamer_FW_"
    )
    string(APPEND _stale_regex
           "|QGC_GST_HAS_|GST_D3D|QGC_GST_VULKAN|_GST_.*_PROG$|GStreamer_(UBUNTU_R_TTF|FONTS_CONF|CA_BUNDLE)$|_xcfw_)"
    )
    get_cmake_property(_cache_vars CACHE_VARIABLES)
    set(_purged 0)
    foreach(_var IN LISTS _cache_vars)
        if(_var MATCHES "${_stale_regex}")
            unset(${_var} CACHE)
            math(EXPR _purged "${_purged} + 1")
        endif()
    endforeach()
    if(_purged GREATER 0)
        message(STATUS "GStreamer: SDK changed; cleared ${_purged} cached GStreamer probe results")
    endif()
    set(_QGC_GST_SDK_FINGERPRINT
        "${fingerprint}"
        CACHE INTERNAL "GStreamer SDK identity the cached probe results belong to"
    )
endfunction()

# _qgc_gst_drop_legacy_pkg_config_args()
# Configures before the derived pkg-config flags became a normal variable FORCE-cached them into
# PKG_CONFIG_ARGN, where they would outlive the SDK they describe (a stale --define-variable=prefix
# after a version bump); strip exactly those flags from the cached value.
function(_qgc_gst_drop_legacy_pkg_config_args)
    set(_args "$CACHE{PKG_CONFIG_ARGN}")
    list(FILTER _args EXCLUDE REGEX "^--(static|dont-define-prefix|define-variable=(prefix|libdir|includedir)=.*)$")
    if(NOT "${_args}" STREQUAL "$CACHE{PKG_CONFIG_ARGN}")
        set_property(CACHE PKG_CONFIG_ARGN PROPERTY VALUE "${_args}")
    endif()
endfunction()
