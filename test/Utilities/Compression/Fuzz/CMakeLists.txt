# ============================================================================
# Compression Fuzzing
# A smoke test that runs the fuzzer's seeds through both fuzz targets in the QGC test build and checks what some of
# them give, and the two libFuzzer harnesses, which QGC_COMPRESSION_LIBFUZZER adds: qgc_compression_stream_fuzzer reads
# its input in memory only, qgc_compression_extraction_fuzzer writes it as files and extracts it. It builds
# QGCCompression with ASan and UBSan, and the bundled libarchive and decoders with ASan.
# ============================================================================

option(
    QGC_COMPRESSION_LIBFUZZER
    "Build the libFuzzer harness of QGCCompression, with it and its bundled libraries under ASan" OFF
)

include(${CMAKE_CURRENT_SOURCE_DIR}/BidAgreement.cmake)

add_executable(qgc_compression_fuzz_smoke BidAgreement.cc QGCCompressionFuzzTargets.cc QGCCompressionFuzzSmoke.cc)
qgc_add_bid_agreement(qgc_compression_fuzz_smoke)
# The stream target compares the module with zlib and liblzma
target_link_libraries(
    qgc_compression_fuzz_smoke
    PRIVATE
        ZLIB::ZLIB
        LibLZMA::LibLZMA
)
add_test(NAME QGCCompressionFuzzSmoke COMMAND qgc_compression_fuzz_smoke "${CMAKE_CURRENT_SOURCE_DIR}/..")
set_tests_properties(
    QGCCompressionFuzzSmoke
    PROPERTIES
        LABELS "Unit;Utilities;Fuzz"
        TIMEOUT 60
)

if(QGC_COMPRESSION_LIBFUZZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC)
        message(FATAL_ERROR "QGC_COMPRESSION_LIBFUZZER requires a Clang driver with libFuzzer and sanitizer runtimes")
    endif()
    target_compile_options(
        QGCCompression PUBLIC
        -fsanitize=fuzzer-no-link,address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(QGCCompression INTERFACE -fsanitize=address,undefined)
    # The bundled libraries, if the build has them. With QGC_USE_SYSTEM_LIBS the system's are not instrumented
    foreach(target archive_static zlibstatic liblzma libzstd_static lz4_static bz2_static)
        if(TARGET ${target})
            target_compile_options(
                ${target} PRIVATE
                -fsanitize=fuzzer-no-link,address
                -fno-omit-frame-pointer
            )
        endif()
    endforeach()

    foreach(_harness IN ITEMS Stream Extraction)
        string(TOLOWER ${_harness} _name)
        add_executable(
            qgc_compression_${_name}_fuzzer BidAgreement.cc QGCCompressionFuzzTargets.cc
                                            QGCCompression${_harness}Fuzzer.cc
        )
        qgc_add_bid_agreement(qgc_compression_${_name}_fuzzer)
        target_link_libraries(
            qgc_compression_${_name}_fuzzer
            PRIVATE
                ZLIB::ZLIB
                LibLZMA::LibLZMA
        )
        target_link_options(qgc_compression_${_name}_fuzzer PRIVATE -fsanitize=fuzzer)
    endforeach()
endif()
