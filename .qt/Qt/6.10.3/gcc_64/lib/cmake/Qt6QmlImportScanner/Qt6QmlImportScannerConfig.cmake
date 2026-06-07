# The functionality that was provided in the Qt5 package was moved to the Qml package.
# Defers to that package and it macros.
include(CMakeFindDependencyMacro)
include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlImportScannerDependencies.cmake")
set("Qt6QmlImportScanner_FOUND" TRUE)
