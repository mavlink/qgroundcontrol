if(TARGET QGCWire)
    return()
endif()
add_library(QGCWire INTERFACE)
add_library(QGC::Wire ALIAS QGCWire)
target_compile_features(QGCWire INTERFACE cxx_std_20)
target_include_directories(QGCWire INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(QGCWire INTERFACE FILE_SET HEADERS FILES LittleEndian.h CRC32.h)
