gsty4menc = library('gsty4menc',
  'gsty4mencode.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstvideo_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gsty4menc, install_dir : plugins_pkgconfig_install_dir)
plugins += [gsty4menc]
