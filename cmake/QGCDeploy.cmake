
if(LINUX)
	# TODO: investigate https://github.com/probonopd/linuxdeployqt

	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/release/package/QGroundControl.AppImage
		COMMAND ${CMAKE_SOURCE_DIR}/deploy/create_linux_appimage.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/release/package;
		DEPENDS QGroundControl
		USES_TERMINAL
	)
	add_custom_target(appimage DEPENDS ${CMAKE_BINARY_DIR}/release/package/QGroundControl.AppImage)

elseif(APPLE)

	get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
	get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
	find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

	add_custom_target(dmg)

	if(GST_FOUND)
		add_custom_command(TARGET dmg
			OUTPUT
			COMMAND
				${CMAKE_SOURCE_DIR}/tools/prepare_gstreamer_framework.sh ${CMAKE_BINARY_DIR}/gstwork/ QGroundControl.app QGroundControl
		)
	endif()

	add_custom_command(TARGET dmg
		OUTPUT
		COMMAND
			${MACDEPLOYQT_EXECUTABLE} $<TARGET_FILE_DIR:QGroundControl>/../.. -appstore-compliant -qmldir=${CMAKE_SOURCE_DIR}/src
		COMMAND
			rsync -a ${CMAKE_SOURCE_DIR}/libs/Frameworks $<TARGET_FILE_DIR:QGroundControl>/../../Contents/
		COMMAND
			${CMAKE_INSTALL_NAME_TOOL} -change "@rpath/SDL2.framework/Versions/A/SDL2" "@executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2" $<TARGET_FILE:QGroundControl>
		COMMAND
			mkdir -p ${CMAKE_BINARY_DIR}/package
		COMMAND
			mkdir -p ${CMAKE_BINARY_DIR}/staging
		COMMAND
			rsync -a --delete ${CMAKE_BINARY_DIR}/QGroundControl.app ${CMAKE_BINARY_DIR}/staging
		COMMAND
			hdiutil create /tmp/tmp.dmg -ov -volname "QGroundControl-$${APP_VERSION_STR}" -fs HFS+ -srcfolder "staging"
		COMMAND
			hdiutil convert /tmp/tmp.dmg -format UDBZ -o ${CMAKE_BINARY_DIR}/package/QGroundControl.dmg
	)

	set_target_properties(QGroundControl PROPERTIES MACOSX_BUNDLE YES)

elseif(WIN32)
	if(MSVC) # Check if we are using the Visual Studio compiler
		set_target_properties(${PROJECT_NAME} PROPERTIES
			WIN32_EXECUTABLE YES
			LINK_FLAGS "/ENTRY:mainCRTStartup"
		)
	endif()

	# deploy
	include(Windeployqt)
	windeployqt(QGroundControl "QGroundControl-installer.exe")

	add_custom_command(TARGET QGroundControl POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E
		copy_if_different $<TARGET_FILE:sdl2> $<TARGET_FILE_DIR:QGroundControl>
	)

elseif(ANDROID)
	include(AddQtAndroidApk)
	add_qt_android_apk(QGroundControl.apk QGroundControl
		PACKAGE_NAME "io.mavlink.qgroundcontrol"
		#KEYSTORE ${CMAKE_CURRENT_LIST_DIR}/mykey.keystore myalias
		#KEYSTORE_PASSWORD xxxxx
	)
endif()
