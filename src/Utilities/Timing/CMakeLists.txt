if(TARGET QGCTiming)
    return()
endif()
if(NOT TARGET QGCClock)
    add_subdirectory(Clock "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()

add_library(QGCTiming STATIC RuntimeScheduler.cc QtRuntimeScheduler.cc ScheduledTask.cc)
add_library(QGC::Timing ALIAS QGCTiming)
set_target_properties(QGCTiming PROPERTIES AUTOMOC ON)
target_compile_features(QGCTiming PUBLIC cxx_std_20)
target_link_libraries(
    QGCTiming
    PUBLIC Qt6::Core QGCClock
    PRIVATE QGCLoggingCategory
)

target_sources(QGCTiming PUBLIC FILE_SET HEADERS FILES RuntimeScheduler.h QtRuntimeScheduler.h ScheduledTask.h)
