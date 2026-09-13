# Synclair Vision custom-build resources and installer branding.
# This file is loaded by QGC before the executable target is created.

list(APPEND QGC_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/custom.qrc"
)

set(CPACK_IFW_PACKAGE_LOGO2 "${CMAKE_CURRENT_SOURCE_DIR}/res/svlogo(2).svg")
