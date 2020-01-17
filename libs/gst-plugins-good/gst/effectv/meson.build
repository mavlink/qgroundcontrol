effect_sources = [
  'gsteffectv.c', 'gstedge.c', 'gstaging.c', 'gstdice.c', 'gstwarp.c',
  'gstshagadelic.c', 'gstvertigo.c', 'gstrev.c', 'gstquark.c', 'gstop.c',
  'gstradioac.c', 'gststreak.c', 'gstripple.c'
]

gsteffectv = library('gsteffectv',
  effect_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gsteffectv, install_dir : plugins_pkgconfig_install_dir)
plugins += [gsteffectv]
