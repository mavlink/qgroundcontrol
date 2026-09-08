target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEASourceManagerTest.cc NMEASourceManagerTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEAStreamSplitterTest.cc NMEAStreamSplitterTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEAPositionSourceTest.cc NMEAPositionSourceTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(NMEASourceManagerTest LABELS Unit RESOURCE_LOCK Settings)
add_qgc_test(NMEAStreamSplitterTest LABELS Unit)
add_qgc_test(NMEAPositionSourceTest LABELS Unit)
