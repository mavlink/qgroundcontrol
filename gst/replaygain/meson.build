replaygain_sources = [
  'gstrganalysis.c',
  'gstrglimiter.c',
  'gstrgvolume.c',
  'replaygain.c',
  'rganalysis.c',
]

gstreplaygain = library('gstreplaygain',
  replaygain_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gst_dep, gstbase_dep, gstpbutils_dep, gstaudio_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstreplaygain, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstreplaygain]

