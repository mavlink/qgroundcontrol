if(TARGET QGCJsonValidation)
    return()
endif()
add_library(QGCJsonValidation STATIC JsonValidation.cc)
add_library(QGC::JsonValidation ALIAS QGCJsonValidation)
target_link_libraries(QGCJsonValidation PUBLIC Qt6::Core)

target_sources(QGCJsonValidation PUBLIC FILE_SET HEADERS FILES JsonValidation.h)
