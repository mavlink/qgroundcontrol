target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTCMFramerTest.cc RTCMFramerTest.h RTCMConformanceTest.cc)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RTCMFramerTest LABELS Unit GPS)
add_qgc_test(RTCMConformanceTest LABELS Unit GPS)
