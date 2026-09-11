if(TARGET QGCTiming)
    return()
endif()

add_library(
    QGCTiming STATIC
    RuntimeScheduler.cc
    RuntimeScheduler.h
    QtRuntimeScheduler.cc
    QtRuntimeScheduler.h
    ScheduledTask.cc
    ScheduledTask.h
    ManualScheduler.cc
    ManualScheduler.h
)
add_library(QGC::Timing ALIAS QGCTiming)
set_target_properties(QGCTiming PROPERTIES AUTOMOC ON)
target_compile_features(QGCTiming PUBLIC cxx_std_20)
target_include_directories(QGCTiming PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCTiming
    PUBLIC Qt6::Core
    PRIVATE QGCLoggingCategory
)
