
add_subdirectory(FactControls)

set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		FactSystemTestBase.cc
		FactSystemTestGeneric.cc
		FactSystemTestPX4.cc
		ParameterManagerTest.cc
	)
endif()

add_library(FactSystem
	Fact.cc
	FactGroup.cc
	FactMetaData.cc
	FactSystem.cc
	FactValueSliderListModel.cc
	ParameterManager.cc
	SettingsFact.cc

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
