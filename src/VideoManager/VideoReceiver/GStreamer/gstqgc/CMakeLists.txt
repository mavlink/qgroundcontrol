add_subdirectory(gstqml6gl)

if(WIN32 AND NOT ANDROID)
    add_subdirectory(gstqml6d3d11)
endif()

if(GStreamer_USE_STATIC_LIBS)
    foreach(_plugin IN LISTS GSTREAMER_PLUGINS)
        if(GST_PLUGIN_${_plugin}_FOUND)
            target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GST_PLUGIN_${_plugin}_FOUND)
        endif()
    endforeach()
endif()

if(TARGET gstqml6gl)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gstqml6gl)
endif()

if(TARGET gstqml6d3d11)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gstqml6d3d11)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_D3D11_SINK)
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
