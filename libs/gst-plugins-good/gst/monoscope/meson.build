gstmonoscope = library('gstmonoscope',
  'gstmonoscope.c',
  'monoscope.c',
  'convolve.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstaudio_dep, gstvideo_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstmonoscope, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstmonoscope]
