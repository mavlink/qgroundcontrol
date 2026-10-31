# The GStreamer libraries (components) the build can request, and their pkg-config modules. The plugins are
# PluginPolicy.cmake's.

include_guard(GLOBAL)

# The GStreamer libraries the build can request, in probe order. As in Qt's FindGStreamer.cmake, a component's target
# is GStreamer::<Component> and its pkg-config module follows from the CamelCase name: GlPrototypes is
# gstreamer-gl-prototypes-1.0. The mandatory ones are always probed and fail configure when missing: the sources include
# their headers unconditionally. Gl/GlPrototypes are optional: a sysroot whose x11 .pc files are incomplete fails their
# pkg-config checks, and only the GL GPU path needs them. Check (gst-check's GstHarness) serves only the unit tests.
set(GSTREAMER_COMPONENTS
    Base
    Gl
    GlPrototypes
    Rtsp
    Video
    App
    RtspServer
    Allocators
    Check
)
set(GSTREAMER_MANDATORY_COMPONENTS Base Rtsp Video)

# gstreamer_component_module(<component> <out_var>)
# Sets <out_var> to the component's gstreamer-<kebab-case>-1.0 pkg-config module.
function(gstreamer_component_module component out_var)
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1-\\2" _module "${component}")
    string(TOLOWER "${_module}" _module)
    set(${out_var}
        "gstreamer-${_module}-1.0"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_select_components(<out_var> <component>...)
# Sets <out_var> to the mandatory components and the given ones, in GSTREAMER_COMPONENTS order; fails on an unknown one.
function(gstreamer_select_components out_var)
    set(_unknown ${ARGN})
    list(REMOVE_ITEM _unknown ${GSTREAMER_COMPONENTS})
    if(_unknown)
        message(FATAL_ERROR "GStreamer: unknown components ${_unknown} (known: ${GSTREAMER_COMPONENTS})")
    endif()
    set(_selected "")
    foreach(_component IN LISTS GSTREAMER_COMPONENTS)
        if(_component IN_LIST GSTREAMER_MANDATORY_COMPONENTS OR _component IN_LIST ARGN)
            list(APPEND _selected ${_component})
        endif()
    endforeach()
    set(${out_var}
        "${_selected}"
        PARENT_SCOPE
    )
endfunction()
