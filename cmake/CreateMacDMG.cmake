message(STATUS "Creating Mac DMG")

set(BUNDLE_PATH ${CMAKE_BINARY_DIR}/staging/${TARGET_APP_NAME}.app)
execute_process(
    COMMAND ${MACDEPLOYQT} "${BUNDLE_PATH}" -appstore-compliant -verbose=1 -qmldir=${CMAKE_SOURCE_DIR}/src
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND codesign --force --deep -s - "${BUNDLE_PATH}"
    COMMAND_ERROR_IS_FATAL ANY
)
if(EXISTS ${CMAKE_BINARY_DIR}/package)
    file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/package)
endif()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/package)
file(COPY ${BUNDLE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/package)
execute_process(
    COMMAND create-dmg --volname "${TARGET_APP_NAME}" --filesystem "APFS" "${TARGET_APP_NAME}.dmg" "${CMAKE_BINARY_DIR}/package/"
    COMMAND_ERROR_IS_FATAL ANY
)
