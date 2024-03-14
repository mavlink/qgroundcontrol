add_subdirectory(FactControls)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(FactSystem STATIC
	Fact.cc
	Fact.h
	FactGroup.cc
	FactGroup.h
	FactMetaData.cc
	FactMetaData.h
	FactSystem.cc
	FactSystem.h
	FactValueSliderListModel.cc
	FactValueSliderListModel.h
	ParameterManager.cc
	ParameterManager.h
	SettingsFact.cc
	SettingsFact.h
)

target_link_libraries(FactSystem
    PUBLIC
		qgc
		FactControls
)

target_include_directories(FactSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
