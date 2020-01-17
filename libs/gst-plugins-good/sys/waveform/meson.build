waveformsink_sources = [
  'gstwaveformsink.c',
  'gstwaveformplugin.c',
]

have_waveform = false
waveform_option = get_option('waveform')
if host_system != 'windows' or waveform_option.disabled()
  subdir_done()
endif

# TODO: https://github.com/mesonbuild/meson/issues/3940
have_waveform = cc.has_header('mmsystem.h', prefix : '#include <windows.h>')
if not have_waveform and waveform_option.enabled()
  error('waveform plugin was enabled but mmsystem.h was not found')
endif

if have_waveform
  gstwaveformsink = library('gstwaveform',
    waveformsink_sources,
    c_args : gst_plugins_good_args,
    include_directories : [configinc],
    dependencies : [gstaudio_dep, cc.find_library('winmm')],
    install : true,
    install_dir : plugins_install_dir)
  pkgconfig.generate(gstwaveformsink, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstwaveformsink]
endif
