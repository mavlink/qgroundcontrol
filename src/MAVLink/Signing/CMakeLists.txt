target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE MAVLinkSigning.cc
            MAVLinkSigning.h
            MAVLinkSigningKeys.cc
            MAVLinkSigningKeys.h
            SigningChannel.cc
            SigningChannel.h
            SigningController.cc
            SigningController.h
            SigningFailure.h
            SigningStatus.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
