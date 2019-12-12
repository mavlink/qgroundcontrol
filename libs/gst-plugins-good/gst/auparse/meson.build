gstauparse = library('gstauparse',
  'gstauparse.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstaudio_dep, gstbase_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstauparse, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstauparse]
