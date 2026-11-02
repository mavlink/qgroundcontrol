# Installing the GStreamer runtime a desktop app bundles: the shared machinery (copying plugins, GIO modules and
# helpers; installing only the SDK libraries they and QGC load, _qgc_gst_install_runtime_closure; failing an install
# that would ship a GPL library or plugin, _qgc_gst_install_gpl_check; verifying the installed plugins) and the
# dispatcher qgc_gst_install_platform_sdk, called from src/VideoManager/GStreamer/CMakeLists.txt after the
# ANDROID/IOS early-return. Each platform's installer is in platform/<platform>.cmake.
# Depends on: Platform.cmake (qgc_gst_platform), Layout.cmake (the Orchestrator's BUNDLED_PLUGINS, HOMEBREW_PREFIX)
#             and PluginPolicy.cmake (qgc_gst_platform_plugin_attrs, GSTREAMER_BUNDLE_REQUIRED_PLUGINS,
#             GSTREAMER_GPL_DENYLIST; included again by the install-time checks).

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Layout.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# _qgc_gst_filter_plugin_paths(<prefix> <paths> <out_var> [<name>...])
# Pure (script-testable) name filter: keep only paths whose basename is
# <prefix><name>(boundary) where <name> is one of the <name>s, by default GSTREAMER_PLUGINS. The trailing
# boundary stops gstvideo matching gstvideoconvert.
function(_qgc_gst_filter_plugin_paths prefix paths out_var)
    set(_names ${ARGN})
    if(NOT _names)
        set(_names ${GSTREAMER_PLUGINS})
    endif()
    set(_filtered "")
    foreach(_path IN LISTS paths)
        get_filename_component(_name "${_path}" NAME)
        foreach(_allowed IN LISTS _names)
            string(REGEX REPLACE "([][.+*?^$()|\\\\])" "\\\\\\1" _allowed_re "${_allowed}")
            if(_name MATCHES "^${prefix}${_allowed_re}([^a-zA-Z0-9]|$)")
                list(APPEND _filtered "${_path}")
                break()
            endif()
        endforeach()
    endforeach()
    set(${out_var}
        "${_filtered}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_install_plugins(<source_dir> <dest_dir> <prefix> <extension>)
# Installs the GSTREAMER_PLUGINS found as <prefix><name>*.<extension> in <source_dir>. Fails when none match: the glob
# runs at configure time, so the SDK must be complete then. Records the files in QGC_GST_SHIPPED_PLUGINS, which the
# CI boot test of a build tree loads instead of the whole SDK, under that public QGC_ name.
# cmake-lint: disable=C0103
function(qgc_gst_install_plugins source_dir dest_dir prefix extension)
    file(GLOB _plugins "${source_dir}/${prefix}*.${extension}")
    _qgc_gst_filter_plugin_paths("${prefix}" "${_plugins}" _plugins)
    if(NOT _plugins)
        message(
            FATAL_ERROR "qgc_gst_install_plugins: no requested plugin matches ${source_dir}/${prefix}*.${extension}. "
                        "Re-run cmake configure against a complete SDK."
        )
    endif()
    _qgc_gst_install_files("${dest_dir}" ${_plugins})
    set(QGC_GST_SHIPPED_PLUGINS
        "${_plugins}"
        CACHE INTERNAL "GStreamer plugin files the package ships"
    )
endfunction()

# qgc_gst_install_gio_modules(<source_dir> <dest_dir> <extension> [<module>...])
# Installs every *.<extension> GIO module in <source_dir>, or only the lib<module>.<extension> ones given.
function(qgc_gst_install_gio_modules source_dir dest_dir extension)
    if(NOT EXISTS "${source_dir}")
        message(WARNING "qgc_gst_install_gio_modules: ${source_dir} does not exist")
        return()
    endif()
    file(GLOB _modules "${source_dir}/*.${extension}")
    if(ARGN)
        _qgc_gst_filter_plugin_paths(lib "${_modules}" _modules ${ARGN})
    endif()
    _qgc_gst_install_files("${dest_dir}" ${_modules})
endfunction()

# _qgc_gst_install_files(<dest_dir> [PROGRAMS] <file>...)
# Installs the <file>s (executables with PROGRAMS) into <dest_dir> and records <dest_dir> for
# _qgc_gst_install_gpl_check. While _QGC_GST_RUNTIME_CLOSURE is set (_qgc_gst_install_runtime_closure), the files also
# seed the runtime dependency set the closure installs: each becomes an imported module or executable installed with
# install(IMPORTED_RUNTIME_ARTIFACTS).
function(_qgc_gst_install_files dest_dir)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "PROGRAMS" "" "")
    set(_files ${ARG_UNPARSED_ARGUMENTS})
    if(NOT _files)
        return()
    endif()
    set_property(GLOBAL APPEND PROPERTY _QGC_GST_INSTALL_DIRS "${dest_dir}")
    if(NOT _QGC_GST_RUNTIME_CLOSURE)
        if(ARG_PROGRAMS)
            install(PROGRAMS ${_files} DESTINATION "${dest_dir}")
        else()
            install(FILES ${_files} DESTINATION "${dest_dir}")
        endif()
        return()
    endif()
    foreach(_file IN LISTS _files)
        get_property(_count GLOBAL PROPERTY _QGC_GST_RUNTIME_FILE_COUNT)
        math(EXPR _count "${_count} + 1")
        set_property(GLOBAL PROPERTY _QGC_GST_RUNTIME_FILE_COUNT ${_count})
        set(_target GStreamer::runtime_file_${_count})
        if(ARG_PROGRAMS)
            add_executable(${_target} IMPORTED)
        else()
            add_library(${_target} MODULE IMPORTED)
        endif()
        set_target_properties(
            ${_target}
            PROPERTIES
                IMPORTED_LOCATION "${_file}"
        )
        install(
            IMPORTED_RUNTIME_ARTIFACTS
            ${_target}
            RUNTIME_DEPENDENCY_SET
            qgc_gstreamer
            RUNTIME
            DESTINATION
            "${dest_dir}"
            LIBRARY
            DESTINATION
            "${dest_dir}"
        )
    endforeach()
endfunction()

# _qgc_gst_linked_sdk_libraries(<out_var>)
# Sets <out_var> to the SDK's shared libraries QGC links through GStreamer::GStreamer: the .dylib files on macOS, and on
# Windows the <name>[-<n>].dll in <root>/bin of each <name>.lib import library.
function(_qgc_gst_linked_sdk_libraries out_var)
    qgc_gst_layout_get(ROOT_DIR _gst_root)
    _qgc_gst_link_closure(_items GStreamer::GStreamer)
    set(_libraries "")
    foreach(_item IN LISTS _items)
        cmake_path(IS_PREFIX _gst_root "${_item}" NORMALIZE _in_sdk)
        if(NOT IS_ABSOLUTE "${_item}" OR NOT _in_sdk)
            continue()
        endif()
        if(_item MATCHES "\\.dylib$")
            list(APPEND _libraries "${_item}")
        elseif(_item MATCHES "\\.lib$")
            cmake_path(GET _item STEM _stem)
            file(GLOB _dlls "${_gst_root}/bin/${_stem}*.dll")
            string(REGEX REPLACE "([][.+*?^$()|\\\\])" "\\\\\\1" _stem_re "${_stem}")
            list(FILTER _dlls INCLUDE REGEX "/${_stem_re}(-[0-9]+)?\\.dll$")
            list(APPEND _libraries ${_dlls})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _libraries)
    set(${out_var}
        "${_libraries}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_begin_runtime_closure()
# Makes the files the caller installs from here on with _qgc_gst_install_files seed the runtime dependency set
# qgc_gstreamer that _qgc_gst_install_runtime_closure installs.
# cmake-lint: disable=C0103
macro(_qgc_gst_begin_runtime_closure)
    set(_QGC_GST_RUNTIME_CLOSURE ON)
endmacro()

# _qgc_gst_install_runtime_closure(<library_dest>)
# Installs into <library_dest> the libraries of the SDK at GStreamer_ROOT_DIR that the seeds installed since
# _qgc_gst_begin_runtime_closure() and the SDK libraries QGC links load, directly or not, instead of every library the
# SDK ships (which include unused codecs, some GPL), and ends the closure in the caller. Libraries loaded with dlopen,
# such as libproxy's backends, must be installed as seeds themselves.
function(_qgc_gst_install_runtime_closure library_dest)
    qgc_gst_layout_get(ROOT_DIR _gst_root)
    qgc_gst_layout_get(LIB_PATH _gst_lib_path)
    _qgc_gst_linked_sdk_libraries(_qgc_gst_linked)
    _qgc_gst_install_files("${library_dest}" ${_qgc_gst_linked})
    set_property(GLOBAL APPEND PROPERTY _QGC_GST_INSTALL_DIRS "${library_dest}")
    set(_QGC_GST_RUNTIME_CLOSURE
        OFF
        PARENT_SCOPE
    )
    string(REGEX REPLACE "([][.+*?^$()|\\\\])" "\\\\\\1" _qgc_gst_root_re "${_gst_root}")
    if(WIN32)
        # DLL lookups also see the lowercased path.
        string(TOLOWER "${_qgc_gst_root_re}" _qgc_gst_root_lower_re)
        set(_qgc_gst_include_res "^${_qgc_gst_root_re}/" "^${_qgc_gst_root_lower_re}/")
    else()
        set(_qgc_gst_include_res "^${_qgc_gst_root_re}/")
    endif()
    # The system's libraries resolve outside the SDK and are excluded; API sets, the MSVC runtime (installed by
    # InstallRequiredSystemLibraries) and macOS system libraries in the dyld cache may not resolve at all.
    # libproxy keeps the backend library its own library links (pxbackend) in lib/libproxy.
    install(
        RUNTIME_DEPENDENCY_SET
        qgc_gstreamer
        DIRECTORIES
        "${_gst_root}/bin"
        "${_gst_lib_path}"
        "${_gst_lib_path}/libproxy"
        PRE_EXCLUDE_REGEXES
        "^api-ms-"
        "^ext-ms-"
        "^(vcruntime|msvcp|concrt|ucrtbase)"
        "^/usr/lib/"
        "^/System/"
        POST_INCLUDE_REGEXES
        ${_qgc_gst_include_res}
        POST_EXCLUDE_REGEXES
        ".*"
        RUNTIME
        DESTINATION
        "${library_dest}"
        LIBRARY
        DESTINATION
        "${library_dest}"
        FRAMEWORK
        DESTINATION
        "${library_dest}"
    )
endfunction()

# _qgc_gst_install_gpl_check()
# Fails the install when a GSTREAMER_GPL_DENYLIST library or plugin landed in a directory the GStreamer installers
# installed into.
function(_qgc_gst_install_gpl_check)
    get_property(_dirs GLOBAL PROPERTY _QGC_GST_INSTALL_DIRS)
    list(REMOVE_DUPLICATES _dirs)
    set(_check_code
        [=[
cmake_policy(PUSH)
cmake_policy(VERSION 3.25)
include("@_policy_file@")
set(_gpl_files "")
foreach(_dir IN ITEMS @_dirs@)
    file(GLOB_RECURSE _files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${_dir}/*")
    qgc_gst_gpl_files(_found ${_files})
    list(APPEND _gpl_files ${_found})
endforeach()
if(_gpl_files)
    list(REMOVE_DUPLICATES _gpl_files)
    list(JOIN _gpl_files "
    " _gpl_list)
    message(FATAL_ERROR "GStreamer install check: the install ships GPL-licensed GStreamer files, which QGC's LGPL "
                        "notice does not cover:
    ${_gpl_list}
Remove them from the plugin allowlist "
                        "(.github/build-config.json) or find what loads them.")
endif()
cmake_policy(POP)
]=]
    )
    set(_policy_file "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PluginPolicy.cmake")
    string(CONFIGURE "${_check_code}" _check_code @ONLY)
    install(CODE "${_check_code}")
endfunction()

# The per-platform installers, qgc_gst_install_<platform>_sdk in platform/<platform>.cmake, are called as
# <helper>(<app_name> <plugin_dest_out>): each sets <plugin_dest_out> to the install-prefix-relative directory it
# installs the plugins to, using the shared helpers above and the SDK layout discovery recorded (qgc_gst_layout_get).

# Installs the LGPL text and notice the Orchestrator wrote (the layout's NOTICE_DIR) into <dest>.
function(_qgc_gst_install_notice dest)
    qgc_gst_layout_get(NOTICE_DIR _notice_dir)
    if(_notice_dir)
        install(FILES "${_notice_dir}/NOTICE.txt" "${_notice_dir}/LGPL-2.1.txt" DESTINATION "${dest}")
    endif()
endfunction()

# Top-level dispatcher — call this from CMakeLists.txt after the ANDROID/IOS guard (mobile platforms link their plugins
# statically and have no install helper). Runs the platform's qgc_gst_install_<platform>_sdk.
# <app_name> is CMAKE_PROJECT_NAME in the caller.
# qgc_gst_install_platform_sdk(<app_name> [REQUIRED_PLUGINS <names…>])
# REQUIRED_PLUGINS: list of plugin basenames (no platform prefix or extension) to
# verify exist post-install; default is the minimum needed to run any pipeline,
# extended with every configured plugin present in the SDK at configure time.
function(qgc_gst_install_platform_sdk app_name)
    if(NOT app_name)
        message(FATAL_ERROR "qgc_gst_install_platform_sdk: app_name is required")
    endif()
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "REQUIRED_PLUGINS")
    _qgc_gst_check_args(qgc_gst_install_platform_sdk)
    qgc_gst_platform(_gips_platform)
    # QGC_GST_BUNDLE_PLUGINS=OFF (unset means ON): a Linux distribution package depends on the system's plugins, which
    # the app then uses as they are (GStreamerEnvironment.cc, QGC_GST_SYSTEM_PLUGINS).
    if(_gips_platform STREQUAL "linux"
       AND DEFINED QGC_GST_BUNDLE_PLUGINS
       AND NOT QGC_GST_BUNDLE_PLUGINS
    )
        message(STATUS "GStreamer: QGC_GST_BUNDLE_PLUGINS is OFF; the install uses the system's plugins")
        return()
    endif()
    # Homebrew's libraries, plugins and plugin scanner load each other by absolute paths under its prefix (no @rpath),
    # so copies bundled into the app would not be the ones loaded.
    qgc_gst_layout_get(HOMEBREW_PREFIX _gips_homebrew_prefix)
    if(_gips_homebrew_prefix)
        set(_gips_message "GStreamer: this build uses Homebrew's GStreamer (${_gips_homebrew_prefix}), which cannot be")
        string(
            APPEND _gips_message " bundled. Configure with the official SDK (QGC_GST_USE_HOMEBREW=OFF and "
            "GStreamer_ROOT_DIR unset) to install or package."
        )
        install(CODE "message(FATAL_ERROR \"${_gips_message}\")")
        return()
    endif()
    if(NOT ARG_REQUIRED_PLUGINS)
        list(APPEND ARG_REQUIRED_PLUGINS ${GSTREAMER_BUNDLE_REQUIRED_PLUGINS})
    endif()

    # Extend verification beyond the structural floor to every configured plugin the SDK had at configure time (the
    # Orchestrator's BUNDLED_PLUGINS), so an install-filter drop fails loudly.
    qgc_gst_layout_get(BUNDLED_PLUGINS _gips_bundled_plugins)
    list(APPEND ARG_REQUIRED_PLUGINS ${_gips_bundled_plugins})
    list(REMOVE_DUPLICATES ARG_REQUIRED_PLUGINS)

    cmake_language(CALL qgc_gst_install_${_gips_platform}_sdk "${app_name}" _verify_dest)
    _qgc_gst_install_gpl_check()

    # Post-install plugin verification, against the plugin directory the installer reported.
    # Checked at install time against $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}, where CPack stages packages. Install
    # scripts run with every policy unset, which PluginPolicy.cmake (IN_LIST) cannot use.
    qgc_gst_platform_plugin_attrs(_verify_ext _verify_prefix)
    set(_verify_code
        [=[
cmake_policy(PUSH)
cmake_policy(VERSION 3.25)
include("@_policy_file@")
set(_verify_dir "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/@_verify_dest@")
file(GLOB _installed_plugins RELATIVE "${_verify_dir}" "${_verify_dir}/@_verify_prefix@*.@_verify_ext@")
list(TRANSFORM _installed_plugins REPLACE "^@_verify_prefix@(.*)\\.@_verify_ext@$" "\\1")
qgc_gst_missing_required(REQUIRED @ARG_REQUIRED_PLUGINS@ AVAILABLE ${_installed_plugins} OUT_VAR _missing_plugins)
if(_missing_plugins)
    list(JOIN _missing_plugins ", " _missing_list)
    message(FATAL_ERROR "GStreamer install verification: missing required plugins in @_verify_dest@: "
                        "${_missing_list} — bundle is missing configured video plugins")
endif()
message(STATUS "GStreamer install verification: all required plugins present in @_verify_dest@")
cmake_policy(POP)
]=]
    )
    set(_policy_file "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PluginPolicy.cmake")
    string(CONFIGURE "${_verify_code}" _verify_code @ONLY)
    install(CODE "${_verify_code}")
endfunction()
