if(TARGET QGCJsonValidation)
    return()
endif()
add_library(QGCJsonValidation STATIC JsonValidation.cc JsonValidation.h)
add_library(QGC::JsonValidation ALIAS QGCJsonValidation)
target_include_directories(QGCJsonValidation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QGCJsonValidation PUBLIC Qt6::Core)

target_sources(QGCJsonValidation PUBLIC FILE_SET HEADERS FILES JsonValidation.h)
