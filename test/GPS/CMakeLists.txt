# ============================================================================
# GPS Unit Tests
# Tests for GPS/GNSS positioning and RTK corrections, mirroring src/GPS. Test helpers live in namespace GPSTest, in
# Support/ beside the suites that share them; includes are rooted here.
# ============================================================================

# Adds each suite, <path>.cc and <path>.h, to the application and registers it with CTest under its file name and
# @a labels. File-local helpers reuse names across suites (Receiver, SCENARIOS, ...), so the suites stay out of unity
# batches. The GPS label is passed explicitly: the directory's label does not reach add_qgc_test(); a check below
# enforces it.
function(qgc_add_gps_suites labels)
    foreach(_suite IN LISTS ARGN)
        get_filename_component(_suite_name "${_suite}" NAME)
        target_sources(
            QGCUnitTests
            PRIVATE
                ${_suite}.cc
                ${_suite}.h
        )
        set_property(SOURCE ${_suite}.cc TARGET_DIRECTORY QGCUnitTests PROPERTY SKIP_UNITY_BUILD_INCLUSION ON)
        add_qgc_test(${_suite_name} LABELS ${labels} GPS)
    endforeach()
endfunction()

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(
    QGCUnitTests
    PRIVATE
        NTRIP/Support/CasterSession.h
        NTRIP/Support/MockNTRIPTransport.h
        NTRIP/Support/NTRIPTestHelpers.h
        NTRIP/Support/NTRIPTlsTestFixtures.h
        NTRIP/Support/ScriptedNTRIPCaster.h
        Qml/Support/GPSPanelFixtures.h
        Receiver/Support/GPSReceiverTestSupport.h
        Receiver/Support/ScriptedBluetooth.h
        Receiver/Support/ScriptedReceiverWorker.h
        Support/GPSTestHelpers.h
)

set(_qgc_gps_suites
    GPSManagerTest
    GPSSettingsBindingsTest
    Connection/GPSConnectionSupervisorTest
    Corrections/GPSCorrectionLossAlertTest
    Corrections/GPSCorrectionManagerTest
    Corrections/GPSCorrectionSelectorTest
    Corrections/RTCMMAVLinkPacketTest
    Corrections/RTCMMAVLinkTest
    Corrections/RTCMMessageCountModelTest
    Corrections/RTCMUdpInputTest
    NTRIP/HTTP/NTRIPConfigurationTest
    NTRIP/HTTP/NTRIPHttpCodecTest
    NTRIP/HTTP/NTRIPHttpSessionTest
    NTRIP/NTRIPCaptureTest
    NTRIP/NTRIPConnectionStatsTest
    NTRIP/NTRIPGgaReporterTest
    NTRIP/NTRIPHttpTransportTest
    NTRIP/NTRIPManagerTest
    NTRIP/NTRIPSettingsTest
    NTRIP/NTRIPSourceTableControllerTest
    NTRIP/NTRIPSourceTableTest
    NTRIP/NTRIPTlsTest
    Positioning/GPSSourceHealthTest
    Positioning/PositionManagerTest
    Qml/GPSBaseStationMapTest
    Qml/GPSCorrectionsPanelTest
    Qml/GPSIndicatorTest
    Qml/GPSPositionReceiverTest
    Qml/GPSReceiverSettingsTest
    Qml/GPSSatelliteViewTest
    Qml/NTRIPMountpointBrowserTest
    Qml/NTRIPSavedCastersTest
    Receiver/GPSBluetoothLinkTest
    Receiver/GPSFixedBaseEcefTest
    Receiver/GPSReceiverBluetoothTest
    Receiver/GPSReceiverConnectionPolicyTest
    Receiver/GPSReceiverFactGroupTest
    Receiver/GPSReceiverPresentationTest
    Receiver/GPSReceiverSessionTest
    Receiver/GPSReceiverTest
    Receiver/GPSSatelliteModelTest
)
qgc_add_gps_suites(Unit ${_qgc_gps_suites})
qgc_add_gps_suites("Integration;Vehicle" Corrections/RTCMMAVLinkVehicleTest NTRIP/NTRIPConnectedVehiclesTest)
# Binds the application's GPS services to the settings, for the rest of its process.
qgc_add_gps_suites(Integration Qml/GPSSettingsPagesTest)

add_subdirectory(Core)
add_subdirectory(NTRIP/HTTP/Fuzz)

# The receiver's Bluetooth suite configures the simulated receiver (Core/Protocols/Hardware) where the build has it.
target_compile_definitions(QGCUnitTests PRIVATE $<$<TARGET_EXISTS:QGCGPSReceiverSimulator>:QGC_GPS_RECEIVER_SIMULATOR>)

# `ctest -L GPS` must select every test registered under this directory.
function(qgc_check_gps_labels dir)
    get_property(
        _tests
        DIRECTORY "${dir}"
        PROPERTY TESTS
    )
    foreach(_test IN LISTS _tests)
        get_property(
            _labels
            TEST "${_test}"
            DIRECTORY "${dir}"
            PROPERTY LABELS
        )
        if(NOT "GPS" IN_LIST _labels)
            message(FATAL_ERROR "GPS test ${_test} lacks the GPS label")
        endif()
    endforeach()
    get_property(
        _subdirs
        DIRECTORY "${dir}"
        PROPERTY SUBDIRECTORIES
    )
    foreach(_subdir IN LISTS _subdirs)
        qgc_check_gps_labels("${_subdir}")
    endforeach()
endfunction()
qgc_check_gps_labels("${CMAKE_CURRENT_SOURCE_DIR}")
