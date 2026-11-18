# ExifUtilityTest reads the test image DSCN0010.jpg from the resources of test/AnalyzeView.
add_qgc_tests(
    APMDataFlash/APMDataFlashUtilityTest
    Exif/ExifUtilityTest
    Json/JsonHelperTest
    Json/JsonParsingTest
    Json/JsonResourceAuditTest
    PX4ULog/PX4ULogUtilityTest
    Wire/WireFieldsTest
    LABELS Unit Utilities
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCWire)
