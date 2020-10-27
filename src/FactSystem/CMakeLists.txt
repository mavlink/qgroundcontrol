
add_subdirectory(FactControls)

set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		FactSystemTestBase.cc
		FactSystemTestBase.h
		FactSystemTestGeneric.cc
		FactSystemTestGeneric.h
		FactSystemTestPX4.cc
		FactSystemTestPX4.h
		ParameterManagerTest.cc
		ParameterManagerTest.h
	)
endif()

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

	FactSystemTest.qml

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
