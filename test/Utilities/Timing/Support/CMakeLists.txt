if(TARGET QGCTestTiming)
    return()
endif()
add_library(QGCTestTiming STATIC ManualScheduler.cc)
add_library(QGC::TestTiming ALIAS QGCTestTiming)
set_target_properties(QGCTestTiming PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCTestTiming
    PUBLIC QGCTiming
    PRIVATE QGCLoggingCategory
)
target_sources(QGCTestTiming PUBLIC FILE_SET HEADERS FILES ManualScheduler.h)
