target_sources(${CMAKE_PROJECT_NAME} PRIVATE ExponentialBackoffTest.cc ExponentialBackoffTest.h MonotonicClockTest.cc
                                             MonotonicClockTest.h ScheduledTaskTest.cc ScheduledTaskTest.h
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCClock QGCTiming)
add_qgc_test(ExponentialBackoffTest LABELS Unit Utilities)
add_qgc_test(MonotonicClockTest LABELS Unit Utilities)
add_qgc_test(ScheduledTaskTest LABELS Unit Utilities)
