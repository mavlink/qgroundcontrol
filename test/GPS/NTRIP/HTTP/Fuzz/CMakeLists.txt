# ============================================================================
# NTRIP HTTP Decoder Fuzzing
# A smoke test that replays the fuzzer's input shapes, in the QGC test build and test/GPS/Standalone, and the
# libFuzzer harness, which only the standalone project builds, with QGC_GPS_LIBFUZZER.
# ============================================================================

add_executable(ntrip_http_decoder_smoke NTRIPHttpDecoderFuzzer.cc NTRIPHttpDecoderSmoke.cc)
target_link_libraries(ntrip_http_decoder_smoke PRIVATE QGC::NTRIPHttp)
add_test(NAME NTRIPHttpDecoderFuzzSmoke COMMAND ntrip_http_decoder_smoke)
set_tests_properties(NTRIPHttpDecoderFuzzSmoke PROPERTIES LABELS "Unit;GPS;Fuzz" TIMEOUT 60)

if(QGC_GPS_LIBFUZZER)
    add_executable(ntrip_http_decoder_fuzzer NTRIPHttpDecoderFuzzer.cc)
    target_link_libraries(ntrip_http_decoder_fuzzer PRIVATE QGC::NTRIPHttp)
    target_link_options(ntrip_http_decoder_fuzzer PRIVATE -fsanitize=fuzzer)
endif()
