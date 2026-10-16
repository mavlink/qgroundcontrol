cmake_minimum_required(VERSION 3.25)
# The generated gst_init_static_plugins() source declares and registers exactly the requested plugins and GIO
# modules, and compiles against the host's GStreamer headers when they are installed.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../StaticInit.cmake")

qgc_test_sandbox(_sandbox static-init)

# Sets <out_var> to the arguments of every "<indent><macro>(<arg>);" line of <source>, in order.
function(_qgc_test_macro_args source indent macro out_var)
    file(STRINGS "${source}" _lines REGEX "^${indent}${macro}\\(")
    list(TRANSFORM _lines REPLACE "^${indent}${macro}\\(([A-Za-z0-9_]+)\\);$" "\\1")
    set(${out_var}
        "${_lines}"
        PARENT_SCOPE
    )
endfunction()

# plugins|GIO modules, comma-separated.
foreach(_case IN ITEMS "coreelements,rtp,videoconvertscale|openssl" "coreelements|" "|")
    qgc_test_split("${_case}" _plugins _gio_modules)
    string(REPLACE "," ";" _plugins "${_plugins}")
    string(REPLACE "," ";" _gio_modules "${_gio_modules}")
    set(_source "${_sandbox}/static_init.c")
    _qgc_gst_write_static_init("${_source}" PLUGINS ${_plugins} GIO_MODULES ${_gio_modules})

    _qgc_test_macro_args("${_source}" "" GST_PLUGIN_STATIC_DECLARE _declared)
    qgc_test_assert_streq("declared plugins for ${_case}" "${_plugins}" "${_declared}")
    _qgc_test_macro_args("${_source}" "    " QGC_REGISTER_STATIC_PLUGIN _registered)
    qgc_test_assert_streq("registered plugins for ${_case}" "${_plugins}" "${_registered}")
    _qgc_test_macro_args("${_source}" "" GST_G_IO_MODULE_DECLARE _declared_modules)
    qgc_test_assert_streq("declared GIO modules for ${_case}" "${_gio_modules}" "${_declared_modules}")
    _qgc_test_macro_args("${_source}" "    " GST_G_IO_MODULE_LOAD _loaded_modules)
    qgc_test_assert_streq("loaded GIO modules for ${_case}" "${_gio_modules}" "${_loaded_modules}")
endforeach()
qgc_test_pass("static init declares and registers exactly the given plugins and GIO modules")

find_program(_qgc_pkg_config NAMES pkg-config pkgconf NO_CACHE)
find_program(_qgc_cc NAMES cc gcc clang NO_CACHE)
if(_qgc_pkg_config AND _qgc_cc)
    execute_process(
        COMMAND "${_qgc_pkg_config}" --cflags gstreamer-1.0 gio-2.0
        RESULT_VARIABLE _cflags_result
        OUTPUT_VARIABLE _cflags
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    )
    if(_cflags_result EQUAL 0)
        separate_arguments(_cflags UNIX_COMMAND "${_cflags}")
        set(_source "${_sandbox}/compile_check.c")
        _qgc_gst_write_static_init("${_source}" PLUGINS coreelements rtp GIO_MODULES openssl)
        execute_process(
            COMMAND "${_qgc_cc}" -fsyntax-only -Werror ${_cflags} "${_source}"
            RESULT_VARIABLE _compile_result
            ERROR_VARIABLE _compile_errors
        )
        qgc_test_assert_streq("generated source compiles: ${_compile_errors}" 0 "${_compile_result}")
        qgc_test_pass("static init compiles against the host GStreamer headers")
    endif()
endif()
