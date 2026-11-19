add_qgc_tests(
    GeoJsonHelperTest
    GeoTest
    KMLDomDocumentTest
    PlanarGeometryTest
    ShapeTest
    LABELS Unit Utilities
)

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(
    ${CMAKE_PROJECT_NAME} "Test_Utilities_Geo_res"
    PREFIX "/unittest"
    FILES
        polygon.prj
        polygon.shp
        polygon.shx
        polyline.kml
        polyline.prj
        polyline.shp
        polyline.shx
        polygon.kml
)
