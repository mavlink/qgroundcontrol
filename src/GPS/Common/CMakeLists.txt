# ============================================================================
# GPS Common
# Lowest-level GPS vocabulary: fix quality, constellations, receiver types and driver reports
# ============================================================================

if(NOT TARGET QGCClock)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing/Clock" "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()

# GPSType.h declares a Q_NAMESPACE, so the headers need moc and a compiled library.
qt_add_library(QGCGPSCommon STATIC)
add_library(QGC::GPSCommon ALIAS QGCGPSCommon)
set_target_properties(
    QGCGPSCommon
    PROPERTIES AUTOMOC ON
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSCommon PUBLIC cxx_std_20)
target_link_libraries(QGCGPSCommon PUBLIC Qt6::Core QGCClock)
target_sources(
    QGCGPSCommon
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSConstellation.h
           GPSDriverReports.h
           GPSEllipsoidPosition.h
           GPSFixQuality.h
           GPSType.h
)
