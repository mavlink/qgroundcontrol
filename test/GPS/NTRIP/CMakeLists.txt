# NTRIP tests.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE MockNTRIPTransport.h
            NTRIPManagerTest.cc
            NTRIPManagerTest.h
            NTRIPSourceTableTest.cc
            NTRIPSourceTableTest.h
            NTRIPHttpTransportTest.cc
            NTRIPHttpTransportTest.h
            NTRIPConnectionStatsTest.cc
            NTRIPConnectionStatsTest.h
            NTRIPGgaProviderTest.cc
            NTRIPGgaProviderTest.h
            NTRIPSourceTableControllerTest.cc
            NTRIPSourceTableControllerTest.h
            Standalone/NTRIPReentrancyTest.cc
            Standalone/NTRIPTlsTest.cc
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(NTRIPManagerTest LABELS Unit)
add_qgc_test(NTRIPSourceTableTest LABELS Unit)
add_qgc_test(NTRIPHttpTransportTest LABELS Unit)
add_qgc_test(NTRIPConnectionStatsTest LABELS Unit)
add_qgc_test(NTRIPGgaProviderTest LABELS Unit)
add_qgc_test(NTRIPSourceTableControllerTest LABELS Unit)
add_qgc_test(NTRIPReentrancyTest LABELS Unit GPS)
add_qgc_test(NTRIPTlsTest LABELS Unit GPS)

# QML_FOREIGN runtime registration can succeed even when tooling sees only opaque stubs.
# Keep these generated-metadata checks out of the Core/Network-only standalone builds.
if(TARGET QGroundControlModule)
    get_target_property(_qgc_qml_output_dir ${CMAKE_PROJECT_NAME} QT_QML_MODULE_OUTPUT_DIRECTORY)
    get_target_property(_qgc_qml_typeinfo ${CMAKE_PROJECT_NAME} QT_QML_MODULE_TYPEINFO)
    add_test(NAME NTRIPQmlMetadata COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/check_qml_metadata.py"
                                           "${_qgc_qml_output_dir}/${_qgc_qml_typeinfo}"
    )
    add_test(
        NAME NTRIPQmlLint
        COMMAND
            "${CMAKE_COMMAND}" -E env --modify "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::qmllint>"
            "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_SOURCE_DIR}/tools/analyze.py" --tool qmllint --qml-build
            --build-dir "${CMAKE_BINARY_DIR}" test/GPS/NTRIP/NTRIPQmlMetadata.qml
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(NTRIPQmlMetadata NTRIPQmlLint PROPERTIES LABELS "Unit;GPS;QML" TIMEOUT 60)
endif()
