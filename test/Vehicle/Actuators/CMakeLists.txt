target_sources(${CMAKE_PROJECT_NAME} PRIVATE ActuatorOutputsTest.cc ActuatorOutputsTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(ActuatorOutputsTest LABELS Unit Vehicle)
