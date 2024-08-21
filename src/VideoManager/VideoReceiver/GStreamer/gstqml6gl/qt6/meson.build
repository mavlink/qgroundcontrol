sources = [
  'gstplugin.cc',
  'gstqt6element.cc',
  'gstqsg6glnode.cc',
  'gstqt6glutility.cc',
  'gstqml6glsink.cc',
  'qt6glitem.cc',
]

moc_headers = [
  'qt6glitem.h',
  'gstqsg6glnode.h',
]

qt6qml_dep = dependency('', required: false)
qt6_option = get_option('qt6')
qt6_method = get_option('qt-method')

if qt6_option.disabled()
  subdir_done()
endif

if not have_gstgl
  if qt6_option.enabled()
    error('qt6 qmlglsink plugin is enabled, but gstreamer-gl-1.0 was not found')
  endif
  subdir_done()
endif

if not add_languages('cpp', native: false, required: qt6_option)
  subdir_done()
endif

qt6_mod = import('qt6')
if not qt6_mod.has_tools()
  if qt6_option.enabled()
    error('qt6 qmlglsink plugin is enabled, but qt specific tools were not found')
  endif
  subdir_done()
endif

qt6qml_dep = dependency('qt6', modules : ['Core', 'Gui', 'Qml', 'Quick'],
                        method: qt6_method, required: qt6_option, static: host_system == 'ios')
if not qt6qml_dep.found()
  subdir_done()
endif

optional_deps = []
qt_defines = []
have_qpa_include = false
have_qt_windowing = false

# Look for the QPA platform native interface header
qpa_header_path = join_paths(qt6qml_dep.version(), 'QtGui')
qpa_header = join_paths(qpa_header_path, 'qpa/qplatformnativeinterface.h')
if cxx.has_header(qpa_header, dependencies : qt6qml_dep)
  qt_defines += '-DHAVE_QT_QPA_HEADER'
  qt_defines += '-DQT_QPA_HEADER=' + '<@0@>'.format(qpa_header)
  have_qpa_include = true
  message('Found QtGui QPA header in ' + qpa_header_path)
endif

# Try to come up with all the platform/winsys combinations that will work

if gst_gl_have_window_x11 and gst_gl_have_platform_glx
  # FIXME: automagic
  qt_defines += ['-DHAVE_QT_X11']
  have_qt_windowing = true
endif

if gst_gl_have_platform_egl
  # Embedded linux (e.g. i.MX6) with or without windowing support
  qt_defines += ['-DHAVE_QT_EGLFS']
  optional_deps += gstglegl_dep
  have_qt_windowing = true
  if have_qpa_include
    # Wayland windowing
    if gst_gl_have_window_wayland
      # FIXME: automagic
      qt6waylandextras = dependency('qt6', modules : ['WaylandClient'], method: qt6_method, required : false)
      if qt6waylandextras.found()
        optional_deps += [qt6waylandextras, gstglwayland_dep]
        qt_defines += ['-DHAVE_QT_WAYLAND']
        have_qt_windowing = true
      endif
    endif
    # Android windowing
#    if gst_gl_have_window_android
      # FIXME: automagic
#      qt5androidextras = dependency('qt5', modules : ['AndroidExtras'], method: qt6_method, required : false)
      # for gl functions in QtGui/qopenglfunctions.h
      # FIXME: automagic
#      glesv2_dep = cc.find_library('GLESv2', required : false)
#      if glesv2_dep.found() and qt5androidextras.found()
#        optional_deps += [qt5androidextras, glesv2_dep]
#        qt_defines += ['-DHAVE_QT_ANDROID']
#        have_qt_windowing = true
        # Needed for C++11 support in Cerbero. People building with Android
        # in some other way need to add the necessary bits themselves.
#        optional_deps += dependency('gnustl', required : false)
#      endif
#    endif
  endif
endif

#if gst_gl_have_platform_wgl and gst_gl_have_window_win32
  # for wglMakeCurrent()
  # FIXME: automagic
#  opengl32_dep = cc.find_library('opengl32', required : false)
#  if opengl32_dep.found()
#    qt_defines += ['-DHAVE_QT_WIN32']
#    optional_deps += opengl32_dep
#    have_qt_windowing = true
#  endif
#endif

if gst_gl_have_window_cocoa and gst_gl_have_platform_cgl
  # FIXME: automagic
  if host_machine.system() == 'darwin'
    qt_defines += ['-DHAVE_QT_MAC']
    have_qt_windowing = true
  endif
endif

if gst_gl_have_window_eagl and gst_gl_have_platform_eagl
  if host_machine.system() == 'ios'
    qt_defines += ['-DHAVE_QT_IOS']
    have_qt_windowing = true
  endif
endif

if have_qt_windowing
  # Build it!
  moc_files = qt6_mod.preprocess(moc_headers : moc_headers)
  gstqml6gl = library('gstqml6', sources, moc_files,
    cpp_args : gst_plugins_good_args + qt_defines,
    link_args : noseh_link_args,
    include_directories: [configinc, libsinc],
    dependencies : [gst_dep, gstvideo_dep, gstgl_dep, gstglproto_dep, qt6qml_dep, optional_deps],
    override_options : ['cpp_std=c++17'],
    install: true,
    install_dir : plugins_install_dir)
  pkgconfig.generate(gstqml6gl, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstqml6gl]
endif
