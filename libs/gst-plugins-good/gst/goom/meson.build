goom_sources = [
  'gstgoom.c',
  'drawmethods.c',
  'sound_tester.c',
  'mathtools.c',
  'lines.c',
  'ifs.c',
  'surf3d.c',
  'tentacle3d.c',
  'v3d.c',
  'convolve_fx.c',
  'flying_stars_fx.c',
  'plugin_info.c',
  'goom_tools.c',
  'config_param.c',
  'filters.c',
  'goom_core.c',
  'graphic.c',
]


gstgoom = library('gstgoom',
  goom_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gst_dep, gstpbutils_dep, gstbase_dep, orc_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstgoom, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstgoom]
