smpte_sources = [
  'gstsmpte.c',
  'gstmask.c',
  'barboxwipes.c',
  'paint.c',
  'gstsmptealpha.c',
  'plugin.c',
]

gstsmpte = library('gstsmpte',
  smpte_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstvideo_dep, gst_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstsmpte, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstsmpte]
