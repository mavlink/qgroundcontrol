target_sources(
    QGCUnitTests
    PRIVATE
        ChecksumsTest.cc
        ChecksumsTest.h
)
add_qgc_test(ChecksumsTest LABELS Unit Utilities)
