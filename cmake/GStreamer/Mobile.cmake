# The mobile GStreamer layouts (Android, iOS), where GStreamer::mobile is the one library that links the SDK's static
# archives: its generated gst_init_static_plugins(), the symbols it keeps for the app, and the check that the app
# carries no second GStreamer core.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# _qgc_gst_write_static_init(<output> PLUGINS <plugin>... GIO_MODULES <module>...)
# Writes the gst_static_plugins.c.in registration source to <output>: gst_init_static_plugins() registers each
# PLUGINS entry (gst_plugin_<plugin>_register) that the registry does not have yet and loads each GIO_MODULES
# entry (g_io_<module>_load). Either list may be empty.
function(_qgc_gst_write_static_init output)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "PLUGINS;GIO_MODULES")
    _qgc_gst_check_args(_qgc_gst_write_static_init ALLOW_EMPTY PLUGINS GIO_MODULES)
    if(NOT output)
        message(FATAL_ERROR "_qgc_gst_write_static_init: an output path is required")
    endif()

    # Template slots of gst_static_plugins.c.in.
    set(plugin_declarations "")
    set(plugin_registrations "")
    set(gio_module_declarations "")
    set(gio_module_loads "")
    foreach(_plugin IN LISTS ARG_PLUGINS)
        string(APPEND plugin_declarations "GST_PLUGIN_STATIC_DECLARE(${_plugin});\n")
        string(APPEND plugin_registrations "    QGC_REGISTER_STATIC_PLUGIN(${_plugin});\n")
    endforeach()
    foreach(_module IN LISTS ARG_GIO_MODULES)
        string(APPEND gio_module_declarations "GST_G_IO_MODULE_DECLARE(${_module});\n")
        string(APPEND gio_module_loads "    GST_G_IO_MODULE_LOAD(${_module});\n")
    endforeach()
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/gst_static_plugins.c.in" "${output}" @ONLY)
endfunction()

# qgc_gst_generate_static_init(TARGET <target> [PLUGINS <plugin>...] [GIO_MODULES <module>...])
# Generates the registration source (see _qgc_gst_write_static_init) into <target>'s binary directory as
# <target>_gst_static_plugins.c and compiles it into <target>.
function(qgc_gst_generate_static_init)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET" "PLUGINS;GIO_MODULES")
    _qgc_gst_check_args(qgc_gst_generate_static_init ALLOW_EMPTY PLUGINS GIO_MODULES)
    if(NOT TARGET "${ARG_TARGET}")
        message(FATAL_ERROR "qgc_gst_generate_static_init: TARGET '${ARG_TARGET}' is not a target")
    endif()

    get_target_property(_binary_dir ${ARG_TARGET} BINARY_DIR)
    set(_source "${_binary_dir}/${ARG_TARGET}_gst_static_plugins.c")
    _qgc_gst_write_static_init(
        "${_source}"
        PLUGINS ${ARG_PLUGINS}
        GIO_MODULES ${ARG_GIO_MODULES}
    )
    target_sources(
        ${ARG_TARGET}
        PRIVATE
            "${_source}"
    )
    # A C++ precompiled header (the application's) cannot be used from this C source.
    set_source_files_properties(
        "${_source}"
        TARGET_DIRECTORY
        ${ARG_TARGET}
        PROPERTIES
            SKIP_PRECOMPILE_HEADERS ON
    )
endfunction()

# Core functions QGC calls that the registered plugins do not necessarily pull into GStreamer::mobile. The app links no
# GStreamer archive, so a missing one fails its link with an undefined-symbol error naming it: add it here.
set(QGC_GST_APP_CORE_SYMBOLS gst_debug_bin_to_dot_data gst_debug_bin_to_dot_file)

# qgc_gst_mobile_keep_symbols(<symbol>...)
# Keeps the GStreamer functions <symbol>... (C names) in GStreamer::mobile, because the app resolves every GStreamer
# symbol from it. Use it for optional libraries QGC calls beyond QGC_GST_APP_CORE_SYMBOLS, such as the RTSP server.
function(qgc_gst_mobile_keep_symbols)
    if(NOT TARGET GStreamer::mobile)
        message(FATAL_ERROR "qgc_gst_mobile_keep_symbols: GStreamer::mobile has not been created")
    endif()
    get_target_property(_target GStreamer::mobile ALIASED_TARGET)
    foreach(_symbol IN LISTS ARGN)
        if(APPLE)
            set(_symbol "_${_symbol}")
        endif()
        target_link_options(${_target} PRIVATE "LINKER:-u,${_symbol}")
    endforeach()
endfunction()

# qgc_gst_add_mobile_library([SOURCES <source>...] [PLUGINS <plugin>...])
# Creates GStreamer::mobile, the SHARED GStreamerMobile library that the platform module links the SDK's archives into:
# its generated gst_init_static_plugins() registers the <plugin>s and loads G_IO_MODULES, and it keeps
# QGC_GST_APP_CORE_SYMBOLS for the app. Linked as C++: OpenH264 needs the C++ runtime.
function(qgc_gst_add_mobile_library)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "SOURCES;PLUGINS")
    _qgc_gst_check_args(qgc_gst_add_mobile_library ALLOW_EMPTY SOURCES PLUGINS)
    add_library(GStreamerMobile SHARED ${ARG_SOURCES})
    add_library(GStreamer::mobile ALIAS GStreamerMobile)
    set_target_properties(
        GStreamerMobile
        PROPERTIES
            LINKER_LANGUAGE CXX
    )
    qgc_gst_generate_static_init(
        TARGET GStreamerMobile
        PLUGINS ${ARG_PLUGINS}
        GIO_MODULES ${G_IO_MODULES}
    )
    qgc_gst_mobile_keep_symbols(${QGC_GST_APP_CORE_SYMBOLS})
endfunction()

# qgc_gst_app_core_symbols(<nm_output> <symbol_prefix> <out_var>)
# Sets <out_var> to the GStreamer core symbols that nm output for the app lists as defined, each C name preceded by
# <symbol_prefix> ("_" for Mach-O). The app must take the core from GStreamer::mobile; a core symbol defined in the app
# means a second core was linked into it.
function(qgc_gst_app_core_symbols nm_output symbol_prefix out_var)
    set(_found "")
    foreach(_symbol IN ITEMS gst_init_check gst_element_get_type gst_plugin_feature_get_type)
        if(nm_output MATCHES "[ \t]${symbol_prefix}${_symbol}(\n|$)")
            list(APPEND _found "${symbol_prefix}${_symbol}")
        endif()
    endforeach()
    set(${out_var}
        "${_found}"
        PARENT_SCOPE
    )
endfunction()
