# The GStreamer:: targets of a pkg-config SDK (every platform but iOS, platform/IOS.cmake), created the way Qt's
# FindGStreamer.cmake (qtmultimedia/cmake) creates them: one pkg_check_modules(IMPORTED_TARGET) per library, with
# pkg-config pointed at the SDK by discovery (PkgConfig.cmake):
#   GStreamer::Core         gstreamer-1.0 >= QGC_GST_REQUESTED_VERSION
#   GStreamer::<Component>  gstreamer-<component>-1.0 for each GSTREAMER_SELECTED_COMPONENTS entry found
#                           (Components.cmake)
#   GStreamer::GStreamer    what QGC links: GStreamer::Core and the components. On Android they link into
#                           GStreamer::mobile instead (platform/Android.cmake), and GStreamer::GStreamer carries only
#                           their usage requirements and GStreamer::mobile.
# pkg-config caches its results under the PC_GStreamer_ prefix, which SdkState.cmake drops when the SDK changes.

include_guard(GLOBAL)

# _qgc_gst_pkg_target(<name> <module> [REQUIRED|QUIET])
# Creates GStreamer::<name>, linking PkgConfig::PC_GStreamer_<name> from pkg-config <module>, when the module is found.
function(_qgc_gst_pkg_target name module)
    if(TARGET GStreamer::${name})
        return()
    endif()
    pkg_check_modules(PC_GStreamer_${name} ${ARGN} IMPORTED_TARGET GLOBAL "${module}")
    set(_imported PkgConfig::PC_GStreamer_${name})
    if(NOT TARGET ${_imported})
        return()
    endif()
    get_target_property(_libraries ${_imported} INTERFACE_LINK_LIBRARIES)
    if(_libraries)
        # FindPkgConfig looks a library up only in the -L directories listed ahead of it, which misses those that
        # pkg-config --static (Android) lists after them; look the rest up in all of the module's directories.
        set(_resolved "")
        foreach(_library IN LISTS _libraries)
            if(NOT IS_ABSOLUTE "${_library}" AND NOT _library MATCHES "^-")
                unset(_path)
                find_library(
                    _path
                    NAMES "${_library}"
                    HINTS ${PC_GStreamer_${name}_LIBRARY_DIRS}
                    NO_DEFAULT_PATH NO_CACHE
                )
                if(_path)
                    set(_library "${_path}")
                endif()
            endif()
            list(APPEND _resolved "${_library}")
        endforeach()
        set(_libraries ${_resolved})
        if(APPLE)
            # The macOS SDK's gstreamer-gl-1.0.pc names gstvulkan-1.0, which that SDK does not ship.
            list(FILTER _libraries EXCLUDE REGEX "^gstvulkan(-1\\.0)?$")
        elseif(ANDROID)
            # The SDK is static; whatever it needs from outside comes from the NDK, whose linker picks the variant
            # (libc.so, not the libc.a find_library may resolve).
            set(_by_name "")
            foreach(_library IN LISTS _libraries)
                cmake_path(IS_PREFIX GStreamer_ROOT_DIR "${_library}" NORMALIZE _in_sdk)
                if(IS_ABSOLUTE "${_library}" AND NOT _in_sdk)
                    cmake_path(GET _library STEM _library)
                    string(REGEX REPLACE "^lib" "" _library "${_library}")
                endif()
                list(APPEND _by_name "${_library}")
            endforeach()
            set(_libraries ${_by_name})
        endif()
        set_target_properties(
            ${_imported}
            PROPERTIES
                INTERFACE_LINK_LIBRARIES "${_libraries}"
        )
    endif()
    add_library(GStreamer::${name} INTERFACE IMPORTED GLOBAL)
    target_link_libraries(GStreamer::${name} INTERFACE ${_imported})
endfunction()

# _qgc_gst_link_closure(<out_var> <item>...)
# Sets <out_var> to the libraries and flags the <item>s (targets, libraries, flags) link, following targets through
# their INTERFACE_LINK_LIBRARIES (and $<LINK_LIBRARY:...> wrappers); a static imported target yields its archive.
function(_qgc_gst_link_closure out_var)
    set(_pending ${ARGN})
    set(_seen "")
    set(_closure "")
    while(_pending)
        list(POP_FRONT _pending _item)
        if(_item MATCHES "^\\$<LINK_LIBRARY:[A-Z_]+,(.+)>$")
            set(_item "${CMAKE_MATCH_1}")
        endif()
        if(_item IN_LIST _seen)
            continue()
        endif()
        list(APPEND _seen "${_item}")
        if(NOT TARGET "${_item}")
            list(APPEND _closure "${_item}")
            continue()
        endif()
        get_target_property(_type "${_item}" TYPE)
        if(_type STREQUAL "STATIC_LIBRARY")
            get_target_property(_location "${_item}" IMPORTED_LOCATION)
            list(APPEND _closure "${_location}")
        endif()
        get_target_property(_libraries "${_item}" INTERFACE_LINK_LIBRARIES)
        if(_libraries)
            list(APPEND _pending ${_libraries})
        endif()
    endwhile()
    set(${out_var}
        "${_closure}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_create_sdk_targets()
# Creates the targets listed above from the SDK at GStreamer_ROOT_DIR, then applies the macOS framework overlay or
# builds GStreamer::mobile. Sets GStreamer_VERSION and, on Android, GSTREAMER_STATIC_PLUGINS in the caller's scope.
# cmake-lint: disable=C0103,E1121
function(_qgc_gst_create_sdk_targets)
    # QGC_GST_REQUESTED_VERSION is the per-platform floor chosen by Orchestrator.cmake: Linux honors the distribution
    # minimum (1.20), the bundled-SDK platforms the SDK version they download, so a downgraded SDK fails configure.
    _qgc_gst_pkg_target(Core "gstreamer-1.0>=${QGC_GST_REQUESTED_VERSION}" REQUIRED)
    set(GStreamer_VERSION "${PC_GStreamer_Core_VERSION}")

    if(ANDROID)
        set(_libraries GStreamer::Static)
    else()
        set(_libraries GStreamer::GStreamer)
    endif()
    add_library(${_libraries} INTERFACE IMPORTED GLOBAL)
    target_link_libraries(${_libraries} INTERFACE GStreamer::Core)
    foreach(_component IN LISTS GSTREAMER_SELECTED_COMPONENTS)
        gstreamer_component_module(${_component} _module)
        # A missing mandatory module fails configure with its own message (Orchestrator.cmake); the others are optional.
        set(_quiet QUIET)
        if(_component IN_LIST GSTREAMER_MANDATORY_COMPONENTS)
            set(_quiet "")
        endif()
        _qgc_gst_pkg_target(${_component} "${_module}" ${_quiet})
        if(TARGET GStreamer::${_component})
            target_link_libraries(${_libraries} INTERFACE GStreamer::${_component})
        endif()
    endforeach()

    set(_outputs GStreamer_VERSION)
    if(GStreamer_USE_FRAMEWORK)
        _qgc_gst_apply_macos_framework_overlay()
    elseif(ANDROID)
        _qgc_gst_create_android_mobile_target()
        list(APPEND _outputs GSTREAMER_STATIC_PLUGINS)
    endif()
    return(PROPAGATE ${_outputs})
endfunction()
