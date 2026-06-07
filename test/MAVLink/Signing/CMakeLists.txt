# ============================================================================
# MAVLink Signing Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MockLinkSigningTest.cc
        MockLinkSigningTest.h
        SigningControllerTest.cc
        SigningControllerTest.h
        SigningStatusTest.cc
        SigningStatusTest.h
        SigningTest.cc
        SigningTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(SigningTest LABELS Unit MAVLink)
add_qgc_test(SigningStatusTest LABELS Unit MAVLink)
add_qgc_test(SigningControllerTest LABELS Unit MAVLink)
add_qgc_test(MockLinkSigningTest LABELS Integration Vehicle MAVLink)
