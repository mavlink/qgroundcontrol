x11_dep = dependency('x11', required : get_option('ximagesrc'))

if x11_dep.found()
  x_args = []
  xshm_dep = dependency('xext', required : get_option('ximagesrc-xshm'))
  # FIXME: should add a 'required' arg to cc.has_function() in Meson and use it here
  if xshm_dep.found() and cc.has_function('XShmAttach', dependencies: xshm_dep)
    x_args += ['-DHAVE_XSHM']
  endif

  xfixes_dep = dependency('xfixes', required : get_option('ximagesrc-xfixes'))
  if xfixes_dep.found()
    x_args += ['-DHAVE_XFIXES']
  endif

  xdamage_dep = dependency('xdamage', required : get_option('ximagesrc-xdamage'))
  if xdamage_dep.found()
    x_args += ['-DHAVE_XDAMAGE']
  endif

  gstximagesrc = library('gstximagesrc',
    'gstximagesrc.c', 'ximageutil.c',
    c_args : gst_plugins_good_args + x_args,
    include_directories : [configinc, libsinc],
    dependencies : [gstbase_dep, gstvideo_dep, x11_dep,
                    xshm_dep, xfixes_dep, xdamage_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstximagesrc, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstximagesrc]
endif
