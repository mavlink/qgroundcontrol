target_sources(${CMAKE_PROJECT_NAME} PRIVATE ScheduledTaskTest.cc ScheduledTaskTest.h)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCTiming)
add_qgc_test(ScheduledTaskTest LABELS Unit)
