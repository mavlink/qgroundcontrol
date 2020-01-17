eq_sources = [
  'gstiirequalizer.c',
  'gstiirequalizernbands.c',
  'gstiirequalizer3bands.c',
  'gstiirequalizer10bands.c',
]

gstequalizer = library('gstequalizer',
  eq_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gstbase_dep, gstaudio_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstequalizer, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstequalizer]

install_data(sources: ['GstIirEqualizer3Bands.prs',
  'GstIirEqualizer10Bands.prs'], install_dir: presetdir)
