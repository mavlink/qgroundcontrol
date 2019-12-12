cairo_dep = dependency('cairo-gobject', version : '>=1.10.0', required : get_option('cairo'))

if cairo_dep.found()
  gstcairo = library('gstcairo',
    'gstcairo.c', 'gstcairooverlay.c',
    c_args : gst_plugins_good_args,
    link_args : noseh_link_args,
    include_directories : [configinc],
    dependencies : [gstbase_dep, gstvideo_dep, cairo_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstcairo, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstcairo]
endif
