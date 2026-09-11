target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEASourceManagerTest.cc NMEASourceManagerTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEAStreamSplitterTest.cc NMEAStreamSplitterTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEAPositionSourceTest.cc NMEAPositionSourceTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(NMEASourceManagerTest LABELS Unit RESOURCE_LOCK Settings)
add_qgc_test(NMEAStreamSplitterTest LABELS Unit)
add_qgc_test(NMEAPositionSourceTest LABELS Unit)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEASatelliteAdapterTest.cc NMEASatelliteAdapterTest.h)
add_qgc_test(NMEASatelliteAdapterTest LABELS Unit)

qt_add_executable(QGCNMEADecoderSessionTest NMEADecoderSessionTest.cc)
target_link_libraries(QGCNMEADecoderSessionTest PRIVATE QGCGPSNMEA Qt6::Test)
add_test(NAME NMEADecoderSessionTest COMMAND QGCNMEADecoderSessionTest)
set_tests_properties(NMEADecoderSessionTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)
