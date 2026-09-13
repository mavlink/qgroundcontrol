if(TARGET QGCClock)
    return()
endif()

add_library(QGCClock INTERFACE)
add_library(QGC::Clock ALIAS QGCClock)
target_compile_features(QGCClock INTERFACE cxx_std_20)
target_sources(QGCClock INTERFACE FILE_SET HEADERS FILES MonotonicClock.h)
