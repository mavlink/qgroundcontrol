include_guard(GLOBAL)

add_library(QGCGPSNMEAProtocol INTERFACE)
target_compile_features(QGCGPSNMEAProtocol INTERFACE cxx_std_20)
target_include_directories(QGCGPSNMEAProtocol INTERFACE "${CMAKE_CURRENT_LIST_DIR}"
                                                        "${CMAKE_CURRENT_LIST_DIR}/../../../Core"
)
target_sources(
    QGCGPSNMEAProtocol
    INTERFACE "${CMAKE_CURRENT_LIST_DIR}/NMEAFields.h" "${CMAKE_CURRENT_LIST_DIR}/NMEASentence.h"
              "${CMAKE_CURRENT_LIST_DIR}/NMEAConstellation.h" "${CMAKE_CURRENT_LIST_DIR}/NMEASatelliteEpoch.h"
)
