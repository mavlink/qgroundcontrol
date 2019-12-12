avi_sources = [
  'gstavi.c',
  'gstavimux.c',
  'gstavidemux.c',
  'gstavisubtitle.c'
]

gstavi = library('gstavi',
  avi_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gst_dep, gstriff_dep, gstaudio_dep, gstvideo_dep, gsttag_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstavi, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstavi]
