# ============================================================================
# Concurrency Unit Tests
# QGC::runOnWorker(): work on a worker thread, its result on the thread of an object
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        WorkerJobTest.cc
        WorkerJobTest.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(WorkerJobTest LABELS Unit Utilities)
