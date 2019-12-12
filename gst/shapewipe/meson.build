gstshapewipe = library('gstshapewipe',
  'gstshapewipe.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gio_dep, gst_dep, gstvideo_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstshapewipe, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstshapewipe]
