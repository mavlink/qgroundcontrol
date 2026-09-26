target_sources(${CMAKE_PROJECT_NAME} PRIVATE NotificationQueueTest.cc OperationRevisionTest.cc)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCReentrancy)
add_qgc_test(NotificationQueueTest LABELS Unit Utilities)
add_qgc_test(OperationRevisionTest LABELS Unit Utilities)
