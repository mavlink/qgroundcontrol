# gst_init_static_plugins() generation for static GStreamer layouts (Android, iOS, desktop static SDKs).

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
    _qgc_gst_write_static_init("${_source}" PLUGINS ${ARG_PLUGINS} GIO_MODULES ${ARG_GIO_MODULES})
    target_sources(${ARG_TARGET} PRIVATE "${_source}")
    # A C++ precompiled header (the application's) cannot be used from this C source.
    set_source_files_properties("${_source}" TARGET_DIRECTORY ${ARG_TARGET} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endfunction()
