audiofx_sources = [
  'audiofx.c',
  'audiopanorama.c',
  'audioinvert.c',
  'audioamplify.c',
  'audiodynamic.c',
  'audiokaraoke.c',
  'audiofxbaseiirfilter.c',
  'audiocheblimit.c',
  'audiochebband.c',
  'audioiirfilter.c',
  'audiofxbasefirfilter.c',
  'audiowsincband.c',
  'audiowsinclimit.c',
  'audiofirfilter.c',
  'audioecho.c',
  'gstscaletempo.c',
  'gststereo.c'
]

orcsrc = 'audiopanoramaorc'
if have_orcc
  orc_h = custom_target(orcsrc + '.h',
    input : orcsrc + '.orc',
    output : orcsrc + '.h',
    command : orcc_args + ['--header', '-o', '@OUTPUT@', '@INPUT@'])
  orc_c = custom_target(orcsrc + '.c',
    input : orcsrc + '.orc',
    output : orcsrc + '.c',
    command : orcc_args + ['--implementation', '-o', '@OUTPUT@', '@INPUT@'])
else
  orc_h = configure_file(input : orcsrc + '-dist.h',
    output : orcsrc + '.h',
    copy : true)
  orc_c = configure_file(input : orcsrc + '-dist.c',
    output : orcsrc + '.c',
    copy : true)
endif

gstaudiofx = library('gstaudiofx',
  audiofx_sources, orc_c, orc_h,
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [orc_dep, gstaudio_dep, gstfft_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstaudiofx, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstaudiofx]
