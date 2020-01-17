vfilter_sources = [
  'plugin.c',
  'gstvideoflip.c',
  'gstvideobalance.c',
  'gstgamma.c',
  'gstvideomedian.c',
]

gstvideofilter = library('gstvideofilter',
  vfilter_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstvideo_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstvideofilter, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstvideofilter]
