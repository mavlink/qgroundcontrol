

####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

####################################################################################

include ( "${CMAKE_CURRENT_LIST_DIR}/libevents.cmake" )

add_library(LibEvents::LibEvents INTERFACE IMPORTED)
set_target_properties(LibEvents::LibEvents PROPERTIES INTERFACE_LINK_LIBRARIES "libevents")
