include_guard(GLOBAL)

add_library(QGCGPSNMEAProtocol INTERFACE)
target_compile_features(QGCGPSNMEAProtocol INTERFACE cxx_std_20)
if(NOT TARGET QGCGPSNativeContracts)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../Contracts" "${CMAKE_CURRENT_BINARY_DIR}/NativeContracts")
endif()
target_link_libraries(QGCGPSNMEAProtocol INTERFACE QGCGPSNativeContracts)
target_include_directories(QGCGPSNMEAProtocol INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
target_sources(
    QGCGPSNMEAProtocol
    INTERFACE "${CMAKE_CURRENT_LIST_DIR}/NMEAFields.h" "${CMAKE_CURRENT_LIST_DIR}/NMEASentence.h"
              "${CMAKE_CURRENT_LIST_DIR}/NMEAConstellation.h" "${CMAKE_CURRENT_LIST_DIR}/NMEASatelliteEpoch.h"
)
