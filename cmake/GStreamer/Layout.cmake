# The GStreamer layout store: what the Orchestrator and discovery resolved, as GLOBAL properties
# QGC_GST_LAYOUT_<KEY> that every directory, the install rules and script-mode tests read through qgc_gst_layout_get.
# Discovery records the SDK it resolved (_qgc_gst_return_discovery): ROOT_DIR, AUTO_DOWNLOADED, USE_FRAMEWORK, LIB_PATH,
# PLUGIN_PATH, INCLUDE_PATH, PLUGIN_SCANNER_PATH, GIO_MODULE_PATH, FRAMEWORK_PATH, XCFRAMEWORK_LIB and IOS_CA_BUNDLE.
# platform/MacOS.cmake adds FRAMEWORK_BUNDLE (the GStreamer.framework the app links) and HOMEBREW_PREFIX (a Homebrew
# GStreamer's prefix); Orchestrator.cmake adds NOTICE_DIR, AVAILABLE_PLUGINS (the plugins the SDK offers the build) and
# BUNDLED_PLUGINS (the requested ones among them, which the build ships). The Orchestrator copies the discovery keys
# into the GStreamer_*/GSTREAMER_* variables of its directory for compatibility (qgc_gst_layout_export).

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# qgc_gst_resolve_layout(SDK_ROOT <root> [LIB_PATH <dir>] [PLUGIN_PATH <dir>] [INCLUDE_PATH <dir>]
#                        [PLUGIN_SCANNER_PATH <dir>] [GIO_MODULE_PATH <dir>] [FRAMEWORK_BUNDLE <bundle>])
# Sets, in the caller's scope (a discovery function, which records them with _qgc_gst_return_discovery),
# GStreamer_ROOT_DIR to the normalized <root> and the SDK layout, each defaulting to the SDK directory layout:
# GSTREAMER_LIB_PATH (<root>/lib), GSTREAMER_PLUGIN_PATH (<lib>/gstreamer-1.0), GSTREAMER_INCLUDE_PATH (<root>/include),
# GSTREAMER_PLUGIN_SCANNER_PATH (<root>/libexec/gstreamer-1.0) and GSTREAMER_GIO_MODULE_PATH (<lib>/gio/modules). Fails
# configure when the root, library, plugin or include directory does not exist; with FRAMEWORK_BUNDLE also sets
# GSTREAMER_FRAMEWORK_PATH and the layout's FRAMEWORK_BUNDLE.
# cmake-lint: disable=C0103,E1121
function(qgc_gst_resolve_layout)
    cmake_parse_arguments(
        PARSE_ARGV 0 ARG ""
        "SDK_ROOT;LIB_PATH;PLUGIN_PATH;INCLUDE_PATH;PLUGIN_SCANNER_PATH;GIO_MODULE_PATH;FRAMEWORK_BUNDLE" ""
    )
    _qgc_gst_check_args(qgc_gst_resolve_layout SDK_ROOT)

    cmake_path(SET _root NORMALIZE "${ARG_SDK_ROOT}")
    set(_lib "${_root}/lib")
    if(ARG_LIB_PATH)
        set(_lib "${ARG_LIB_PATH}")
    endif()
    set(_defaults "PLUGIN_PATH|${_lib}/gstreamer-1.0" "INCLUDE_PATH|${_root}/include"
                  "PLUGIN_SCANNER_PATH|${_root}/libexec/gstreamer-1.0" "GIO_MODULE_PATH|${_lib}/gio/modules"
    )
    foreach(_default IN LISTS _defaults)
        string(REPLACE "|" ";" _default "${_default}")
        list(GET _default 0 _key)
        list(GET _default 1 _value)
        if(NOT ARG_${_key})
            set(ARG_${_key} "${_value}")
        endif()
    endforeach()
    foreach(_path IN ITEMS "${_root}" "${_lib}" "${ARG_PLUGIN_PATH}" "${ARG_INCLUDE_PATH}")
        if(NOT EXISTS "${_path}")
            message(
                FATAL_ERROR "GStreamer: SDK path does not exist: ${_path}\n"
                            "Check the installation or set GStreamer_ROOT_DIR."
            )
        endif()
    endforeach()
    set(GStreamer_ROOT_DIR "${_root}")
    set(GSTREAMER_LIB_PATH "${_lib}")
    foreach(_key IN ITEMS PLUGIN_PATH INCLUDE_PATH PLUGIN_SCANNER_PATH GIO_MODULE_PATH)
        set(GSTREAMER_${_key} "${ARG_${_key}}")
    endforeach()
    set(_outputs GStreamer_ROOT_DIR GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH GSTREAMER_INCLUDE_PATH
                 GSTREAMER_PLUGIN_SCANNER_PATH GSTREAMER_GIO_MODULE_PATH
    )
    if(ARG_FRAMEWORK_BUNDLE)
        set(GSTREAMER_FRAMEWORK_PATH "${ARG_FRAMEWORK_BUNDLE}")
        list(APPEND _outputs GSTREAMER_FRAMEWORK_PATH)
        qgc_gst_layout_set(FRAMEWORK_BUNDLE "${ARG_FRAMEWORK_BUNDLE}")
    endif()
    return(PROPAGATE ${_outputs})
endfunction()

# qgc_gst_layout_get(<key> <out_var>)
# Sets <out_var> to the layout's <key>, or "" when it was never set.
function(qgc_gst_layout_get key out_var)
    get_property(_value GLOBAL PROPERTY "QGC_GST_LAYOUT_${key}")
    set(${out_var}
        "${_value}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_layout_set(<key> <value>)
# Sets the layout's <key>, also in script mode.
function(qgc_gst_layout_set key value)
    set_property(GLOBAL PROPERTY "QGC_GST_LAYOUT_${key}" "${value}")
endfunction()

# The discovery keys and the variables qgc_gst_layout_export copies them into.
set(_QGC_GST_DISCOVERY_KEYS
    "ROOT_DIR|GStreamer_ROOT_DIR"
    "AUTO_DOWNLOADED|GStreamer_AUTO_DOWNLOADED"
    "USE_FRAMEWORK|GStreamer_USE_FRAMEWORK"
    "LIB_PATH|GSTREAMER_LIB_PATH"
    "PLUGIN_PATH|GSTREAMER_PLUGIN_PATH"
    "INCLUDE_PATH|GSTREAMER_INCLUDE_PATH"
    "PLUGIN_SCANNER_PATH|GSTREAMER_PLUGIN_SCANNER_PATH"
    "GIO_MODULE_PATH|GSTREAMER_GIO_MODULE_PATH"
    "FRAMEWORK_PATH|GSTREAMER_FRAMEWORK_PATH"
    "XCFRAMEWORK_LIB|GSTREAMER_XCFRAMEWORK_LIB"
    "IOS_CA_BUNDLE|GStreamer_IOS_CA_BUNDLE"
)

# qgc_gst_layout_record_discovery()
# Records every discovery key from the caller's variable of the same name (see _QGC_GST_DISCOVERY_KEYS); "" for one it
# left unset, so a discovery never inherits another's value.
macro(qgc_gst_layout_record_discovery)
    foreach(_qgc_gst_layout_entry IN LISTS _QGC_GST_DISCOVERY_KEYS)
        string(REPLACE "|" ";" _qgc_gst_layout_entry "${_qgc_gst_layout_entry}")
        list(GET _qgc_gst_layout_entry 0 _qgc_gst_layout_key)
        list(GET _qgc_gst_layout_entry 1 _qgc_gst_layout_var)
        qgc_gst_layout_set(${_qgc_gst_layout_key} "${${_qgc_gst_layout_var}}")
    endforeach()
endmacro()

# qgc_gst_layout_export()
# Sets, in the caller's scope, the GStreamer_*/GSTREAMER_* variable of every discovery key to its recorded value: the
# compatibility copies the Orchestrator leaves in its directory. The GStreamer modules read the layout instead.
function(qgc_gst_layout_export)
    set(_outputs "")
    foreach(_entry IN LISTS _QGC_GST_DISCOVERY_KEYS)
        string(REPLACE "|" ";" _entry "${_entry}")
        list(GET _entry 0 _key)
        list(GET _entry 1 _var)
        qgc_gst_layout_get(${_key} ${_var})
        list(APPEND _outputs ${_var})
    endforeach()
    return(PROPAGATE ${_outputs})
endfunction()

# _qgc_gst_return_discovery()
# Returns from a discovery function: records its discovery keys (qgc_gst_layout_record_discovery) and sets, in the
# caller's scope, the pkg-config variables FindPkgConfig reads (PKG_CONFIG_EXECUTABLE, PKG_CONFIG_ARGN,
# PKG_CONFIG_USE_CMAKE_PREFIX_PATH).
macro(_qgc_gst_return_discovery)
    qgc_gst_layout_record_discovery()
    return(PROPAGATE PKG_CONFIG_EXECUTABLE PKG_CONFIG_ARGN PKG_CONFIG_USE_CMAKE_PREFIX_PATH)
endmacro()
