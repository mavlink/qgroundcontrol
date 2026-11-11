target_sources(
    QGCUnitTests
    PRIVATE
        QGCSensorsTest.cc
        QGCSensorsTest.h
)

add_qgc_test(QGCSensorsTest LABELS Unit Utilities)
