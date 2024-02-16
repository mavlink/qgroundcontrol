sources = [
  'gstplugin.cc',
  'gstqt6element.cc',
  'gstqsg6material.cc',
  'gstqt6glutility.cc',
  'gstqml6glsink.cc',
  'gstqml6glsrc.cc',
  'gstqml6glmixer.cc',
  'gstqml6gloverlay.cc',
  'qt6glitem.cc',
  'qt6glwindow.cc',
  'qt6glrenderer.cc',
]

moc_headers = [
  'qt6glitem.h',
  'qt6glwindow.h',
  'qt6glrenderer.h',
]

shader_sources = [
  'vertex.vert',
  'RGBA.frag',
  'YUV_TRIPLANAR.frag',
]

qt6qml_dep = dependency('', required: false)
qt6_option = get_option('qt6')
qt6_egl = get_option('qt-egl')
qt6_wayland = get_option('qt-wayland')
qt6_x11 = get_option('qt-x11')
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
if not qt6_mod.has_tools(method: qt6_method)
  if qt6_option.enabled()
    error('qt6 qmlglsink plugin is enabled, but qt specific tools were not found')
  endif
  subdir_done()
endif

qt6qml_dep = dependency('qt6', modules : ['Core', 'Gui', 'Qml', 'Quick'],
                        method: qt6_method, required: qt6_option, static: host_system == 'ios', private_headers: true)
if not qt6qml_dep.found()
  subdir_done()
endif

qt6_bindir = qt6qml_dep.get_variable('bindir', configtool: 'QT_HOST_BINS')
qsb = find_program('qsb-qt6', 'qsb', dirs: [qt6_bindir], required: qt6_option)
if not qsb.found()
  subdir_done()
endif

optional_deps = []
qt_defines = []
have_qpa_include = false
have_qt_windowing = false

# Look for the QPA platform native interface header
qpa_header_path = join_paths(qt6qml_dep.version(), 'QtGui')
qpa_header = join_paths(qpa_header_path, 'qpa/qplatformnativeinterface.h')
need_qpa_include = qt6_option.enabled() and (host_system == 'android' or qt6_wayland.enabled())
if cxx.has_header(qpa_header, dependencies : qt6qml_dep, required: need_qpa_include)
  qt_defines += '-DHAVE_QT_QPA_HEADER'
  qt_defines += '-DQT_QPA_HEADER=' + '<@0@>'.format(qpa_header)
  have_qpa_include = true
  message('Found QtGui QPA header in ' + qpa_header_path)
endif

## Try to come up with all the platform/winsys combinations that will work

# X11 windowing
qt6_x11 = qt6_x11 \
    .require(gstglx11_dep.found(), error_message: 'gstreamer-gl-x11-1.0 is required') \
    .require(gst_gl_have_window_x11, error_message: 'x11 windowing support in gstreamer-gl is required') \
    .require(gst_gl_have_platform_glx, error_message: 'glx platform support in gstreamer-gl is required')
if qt6_x11.allowed()
  qt_defines += ['-DHAVE_QT_X11']
  have_qt_windowing = true
endif

# Wayland windowing
qt6_wayland = qt6_wayland \
    .require(gstglwayland_dep.found(), error_message: 'gstreamer-gl-wayland-1.0 is required') \
    .require(gst_gl_have_window_wayland, error_message: 'wayland windowing support in gstreamer-gl is required') \
    .require(gst_gl_have_platform_egl, error_message: 'egl platform support in gstreamer-gl is required') \
    .require(have_qpa_include, error_message: 'QPA platform native interface header is required')
if qt6_wayland.allowed()
  qt6waylandextras = dependency('qt6', modules : ['WaylandClient'], method: qt6_method, required: qt6_wayland)
  if qt6waylandextras.found()
    optional_deps += [qt6waylandextras, gstglwayland_dep]
    qt_defines += ['-DHAVE_QT_WAYLAND']
    have_qt_windowing = true
  endif
endif

# EGL windowing for Embedded linux (e.g. i.MX6) with or without windowing
# support
qt6_egl = qt6_egl \
    .require(host_system == 'linux') \
    .require(gstglegl_dep.found(), error_message: 'gstreamer-gl-egl-1.0 is required') \
    .require(gst_gl_have_platform_egl, error_message: 'egl platform support in gstreamer-gl is required')
if qt6_egl.allowed()
  qt_defines += ['-DHAVE_QT_EGLFS']
  optional_deps += gstglegl_dep
  have_qt_windowing = true
endif

# TODO: Android windowing

# Win32 windowing
if host_system == 'windows'
  qt6_win32 = qt6_option \
      .require(gst_gl_have_window_win32, error_message: 'win32 windowing support in gstreamer-gl is required') \
      .require(gst_gl_have_platform_wgl, error_message: 'wgl platform support in gstreamer-gl is required')
  if qt6_win32.allowed()
    # for wglMakeCurrent()
    opengl32_dep = cc.find_library('opengl32', required : qt6_win32)
    if opengl32_dep.found()
      qt_defines += ['-DHAVE_QT_WIN32']
      optional_deps += opengl32_dep
      have_qt_windowing = true
    endif
  endif
endif

# macOS windowing
if host_system == 'darwin'
  qt6_macos = qt6_option \
      .require(gst_gl_have_window_cocoa, error_message: 'cocoa windowing support in gstreamer-gl is required') \
      .require(gst_gl_have_platform_cgl, error_message: 'cgl platform support in gstreamer-gl is required')
  if qt6_macos.allowed()
    qt_defines += ['-DHAVE_QT_MAC']
    have_qt_windowing = true
  endif
endif

# iOS windowing
if host_system == 'ios'
  qt6_ios = qt6_option \
      .require(gst_gl_have_window_eagl, error_message: 'eagl windowing support in gstreamer-gl is required') \
      .require(gst_gl_have_platform_eagl, error_message: 'eagl platform support in gstreamer-gl is required')
  if qt6_ios.allowed()
    qt_defines += ['-DHAVE_QT_IOS']
    have_qt_windowing = true
  endif
endif

if qt6_option.require(have_qt_windowing, error_message: 'No windowing, enable one of the qt-* windowing options').allowed()
  # Build it!
  moc_files = qt6_mod.preprocess(moc_headers : moc_headers, method: qt6_method)
  # TODO: dist backup qsb shaders?
  shaders = []
  foreach shader: shader_sources
    qsb_shader = shader + '.qsb'
    dist_shader = shader + '-dist.qsb'

    compiled_shader = custom_target(qsb_shader,
      input: shader,
      output: qsb_shader,
      command: [qsb, '--glsl=100 es,120,330', '--batchable', '--output', '@OUTPUT@', '@INPUT@']
    )
    shaders += [compiled_shader]
  endforeach
  resource_file = configure_file(input: 'resources.qrc', output: 'resources.qrc', copy: true)
  qresources = qt6_mod.compile_resources(sources: resource_file, method: qt6_method)

  gstqml6gl = library('gstqml6', sources, moc_files, qresources,
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
