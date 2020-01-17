flxdec = library('gstflxdec',
  'gstflxdec.c', 'flx_color.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstvideo_dep, gst_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(flxdec, install_dir : plugins_pkgconfig_install_dir)
plugins += [flxdec]
