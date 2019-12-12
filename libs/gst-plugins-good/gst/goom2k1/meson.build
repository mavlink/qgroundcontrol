goom2k1_args = [
  '-Dgst_goom_get_type=gst_goom2k1_get_type',
  '-Dgoom_init=goom2k1_init',
  '-Dgoom_close=goom2k1_close',
  '-Dgoom_update=goom2k1_update',
  '-Dgoom_set_resolution=goom2k1_set_resolution',
  '-Dgoom_lines=goom2k1_lines',
  '-DBLACK=GOOM2K1_BLACK',
  '-DWHITE=GOOM2K1_WHITE',
  '-DRED=GOOM2K1_RED',
  '-DBLUE=GOOM2K1_BLUE',
  '-DGREEN=GOOM2K1_GREEN',
  '-DYELLOW=GOOM2K1_YELLOW',
  '-DORANGE=GOOM2K1_ORANGE',
  '-DVIOLET=GOOM2K1_VIOLET',
  '-DzoomFilterFastRGB=zoomFilterFastRGB2k1',
  '-DpointFilter=pointFilter2k1',
  '-DzoomFilterDestroy=zoomFilterDestroy2k1',
  '-DzoomFilterNew=zoomFilterNew2k1'
]

filter_args = ['-UMMX', '-UUSE_ASM']

goom2k1_sources = [
  'gstgoom.c',
  'goom_core.c',
  'filters.c',
  'graphic.c',
  'lines.c'
]

gstgoom2k1 = library('gstgoom2k1',
  goom2k1_sources,
  c_args : gst_plugins_good_args + goom2k1_args + filter_args,
  include_directories : [configinc],
  dependencies : [gstpbutils_dep, gstbase_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstgoom2k1, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstgoom2k1]
