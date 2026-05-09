# Compiled as object files into the main exe; gst_plugin_register_static() replaces gst-plugin-scanner
# discovery, so qgcvideosinkbin is available only after gst_init() walks the static plugin list.

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
