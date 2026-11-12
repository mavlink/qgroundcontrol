target_sources(
    QGCUnitTests
    PRIVATE
        QGCFormatTest.cc
        QGCFormatTest.h
)
add_qgc_test(QGCFormatTest LABELS Unit Utilities)
