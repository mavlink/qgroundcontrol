find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(VehicleComponents STATIC
	CompInfo.cc
	CompInfo.h
	CompInfoActuators.cc
	CompInfoActuators.h
	CompInfoEvents.cc
	CompInfoEvents.h
	CompInfoGeneral.cc
	CompInfoGeneral.h
	CompInfoParam.cc
	CompInfoParam.h
	ComponentInformationCache.cc
	ComponentInformationCache.h
	ComponentInformationManager.cc
	ComponentInformationManager.h
	ComponentInformationTranslation.cc
	ComponentInformationTranslation.h
)

target_link_libraries(VehicleComponents
	PRIVATE
	    Compression
		FirmwarePlugin
		QGC
		Vehicle
	PUBLIC
		Qt6::Core
		Comms
		FactSystem
		Utilities
)

target_include_directories(VehicleComponents PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
