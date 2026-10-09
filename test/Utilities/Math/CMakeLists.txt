target_sources(${CMAKE_PROJECT_NAME} PRIVATE ChecksumsTest.cc ChecksumsTest.h)
add_qgc_test(ChecksumsTest LABELS Unit Utilities)
