gstalpha = library('gstalpha', 'gstalpha.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstvideo_dep, gst_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstalpha, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstalpha]

gstalphacolor = library('gstalphacolor', 'gstalphacolor.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstvideo_dep, gst_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstalphacolor, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstalphacolor]
