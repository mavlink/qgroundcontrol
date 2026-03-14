# ============================================================================
# NTRIP Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MockNTRIPTransport.h
        NTRIPConnectionStatsTest.cc
        NTRIPConnectionStatsTest.h
        NTRIPGgaProviderTest.cc
        NTRIPGgaProviderTest.h
        NTRIPHttpTransportTest.cc
        NTRIPHttpTransportTest.h
        NTRIPEndToEndTest.cc
        NTRIPEndToEndTest.h
        NTRIPIntegrationTest.cc
        NTRIPIntegrationTest.h
        NTRIPManagerTest.cc
        NTRIPManagerTest.h
        NTRIPReconnectPolicyTest.cc
        NTRIPReconnectPolicyTest.h
        NTRIPSourceTableTest.cc
        NTRIPSourceTableTest.h
        NTRIPUdpForwarderTest.cc
        NTRIPUdpForwarderTest.h
        RTCMParserTest.cc
        RTCMParserTest.h
        SettingsLifecycleTest.cc
        SettingsLifecycleTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
