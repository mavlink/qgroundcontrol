
add_subdirectory(FactControls)

add_library(FactSystem
	Fact.cc
	FactGroup.cc
	FactGroup.h
	Fact.h
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

	${EXTRA_SRC}
)

target_link_libraries(FactSystem
	PRIVATE
		qgc
		FactControls
)

target_include_directories(FactSystem
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC
		FactControls
)
