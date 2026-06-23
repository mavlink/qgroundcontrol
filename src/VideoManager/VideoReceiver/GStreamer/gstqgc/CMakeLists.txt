# Compiled as object files into the main exe; gst_plugin_register_static() replaces gst-plugin-scanner
# discovery, so qgcvideosinkbin is available only after gst_init() walks the static plugin list.

if(GStreamer_USE_STATIC_LIBS)
    foreach(_plugin IN LISTS GSTREAMER_PLUGINS)
        if(GST_PLUGIN_${_plugin}_FOUND)
            target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GST_PLUGIN_${_plugin}_FOUND)
        endif()
    endforeach()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        gstqgc.cc
        gstqgcelement.cc
        gstqgcelements.h
        gstqgcvideosinkbin.cc
        gstqgcvideosinkbin.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_STREAMING)
