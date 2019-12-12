lame_option = get_option('lame')
lame_dep = cc.find_library('mp3lame', required : lame_option)
have_lame = lame_dep.found()

# TODO: https://github.com/mesonbuild/meson/issues/3940
if have_lame
  have_lame = cc.has_header_symbol('lame/lame.h', 'lame_init')
  if not have_lame and lame_option.enabled()
    error('Found libmp3lame but lame.h was not usable')
  endif
endif

if have_lame
  lame_extra_c_args = []
  if cc.has_header_symbol('lame/lame.h', 'lame_set_VBR_quality')
    lame_extra_c_args += ['-DHAVE_LAME_SET_VBR_QUALITY']
  endif
  if cc.has_header_symbol('lame/lame.h', 'MEDIUM')
    lame_extra_c_args += ['-DGSTLAME_PRESET']
  endif
  lame = library('gstlame',
    ['gstlamemp3enc.c', 'plugin.c'],
    c_args : gst_plugins_good_args + lame_extra_c_args,
    include_directories : [configinc, libsinc],
    dependencies : [gstaudio_dep, lame_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(lame, install_dir : plugins_pkgconfig_install_dir)
  plugins += [lame]
endif
