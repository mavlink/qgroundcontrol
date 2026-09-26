# Runtime framework suites. qgc_add_protocol_test() comes from the parent directory.
foreach(
    suite
    GPSCommandChannelTest
    GPSCommandSequenceTest
    GPSNMEAStreamTest
    GPSProtocolRuntimeTest
    GPSReceiverDetectorTest
    GPSStreamDemuxTest
    GPSTaskTest
)
    qgc_add_protocol_test(${suite} ${suite}.cc)
endforeach()
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSRuntimeTestSupport.h ToyFamily.h)
set_property(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/GPSNMEAStreamTest.cc" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY COMPILE_DEFINITIONS GPS_PROTOCOL_CORPUS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../corpus"
             GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../fixtures"
)
set_property(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/GPSCommandChannelTest.cc" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY COMPILE_DEFINITIONS GPS_CHANNEL_TRANSCRIPT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/transcripts"
)
