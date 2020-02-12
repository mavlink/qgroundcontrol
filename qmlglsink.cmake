
find_package(Qt5Gui ${QT_VERSION} CONFIG REQUIRED Private)

add_library(gst_plugins_good
    libs/gst-plugins-good/ext/qt/gstplugin.cc
    libs/gst-plugins-good/ext/qt/gstqtglutility.cc
    libs/gst-plugins-good/ext/qt/gstqsgtexture.cc
    libs/gst-plugins-good/ext/qt/gstqtsink.cc
    libs/gst-plugins-good/ext/qt/gstqtsrc.cc
    libs/gst-plugins-good/ext/qt/qtwindow.cc
    libs/gst-plugins-good/ext/qt/qtitem.cc

    libs/gst-plugins-good/ext/qt/gstqsgtexture.h
    libs/gst-plugins-good/ext/qt/gstqtgl.h
    libs/gst-plugins-good/ext/qt/gstqtglutility.h
    libs/gst-plugins-good/ext/qt/gstqtsink.h
    libs/gst-plugins-good/ext/qt/gstqtsrc.h
    libs/gst-plugins-good/ext/qt/qtwindow.h
    libs/gst-plugins-good/ext/qt/qtitem.h
)

if(LINUX)
	target_compile_definitions(gst_plugins_good PUBLIC HAVE_QT_X11 HAVE_QT_EGLFS HAVE_QT_WAYLAND)


	find_package(Qt5 ${QT_VERSION}
		COMPONENTS
			X11Extras
		REQUIRED
		HINTS
			${QT_LIBRARY_HINTS}
	)

	target_link_libraries(gst_plugins_good
		PUBLIC
			Qt5::X11Extras
	)

elseif(APPLE)
	target_compile_definitions(gst_plugins_good PUBLIC HAVE_QT_MAC)
elseif(IOS)
	target_compile_definitions(gst_plugins_good PUBLIC HAVE_QT_MAC)
elseif(WIN32)
	target_compile_definitions(gst_plugins_good PUBLIC HAVE_QT_WIN32 HAVE_QT_QPA_HEADER)

	# TODO: use FindOpenGL?
	target_link_libraries(gst_plugins_good PUBLIC opengl32.lib user32.lib)
	# LIBS += opengl32.lib user32.lib
elseif(ANDROID)
	target_compile_definitions(gst_plugins_good PUBLIC HAVE_QT_ANDROID)
endif()

target_link_libraries(gst_plugins_good
	PUBLIC
		Qt5::Core
		Qt5::OpenGL
                Qt5::GuiPrivate
)

target_compile_options(gst_plugins_good
	PRIVATE
		-Wno-unused-parameter
		-Wno-implicit-fallthrough
)

