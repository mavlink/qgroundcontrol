message(STATUS "Signing bundle: ${QGC_BUNDLE_PATH}")
execute_process(
    COMMAND codesign --force --deep -s - "${QGC_BUNDLE_PATH}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/package)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/package)
file(COPY ${QGC_BUNDLE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/package)

message(STATUS "Creating DMG: ${TARGET_APP_NAME}.dmg")
execute_process(
    COMMAND create-dmg --volname "${TARGET_APP_NAME}" --filesystem "APFS" "${TARGET_APP_NAME}.dmg" "${CMAKE_BINARY_DIR}/package/"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)
