directsoundsink_sources = [
  'gstdirectsoundsink.c',
  'gstdirectsounddevice.c',
  'gstdirectsoundplugin.c',
]

directsoundsink_device_flags = [
  '-DGstDirectSoundDeviceProvider=GstDirectSoundSinkDeviceProvider',
  '-DGstDirectSoundDeviceProviderClass=GstDirectSoundSinkDeviceProviderClass',
  '-DGstDirectSoundDevice=GstDirectSoundSinkDevice',
  '-DGstDirectSoundDeviceClass=GstDirectSoundSinkDeviceClass',
]

have_dsound = false
dsound_option = get_option('directsound')
if host_system != 'windows' or dsound_option.disabled()
  subdir_done()
endif

# TODO: https://github.com/mesonbuild/meson/issues/3940
have_dsound = cc.has_header('dsound.h')
if not have_dsound and dsound_option.enabled()
  error('directsound plugin was enabled but dsound.h was not found')
endif

if have_dsound
  directsoundsink_dep = [cc.find_library('dsound'), cc.find_library('winmm'), cc.find_library('ole32')]

  gstdirectsoundsink = library('gstdirectsound',
    directsoundsink_sources,
    c_args : gst_plugins_good_args + directsoundsink_device_flags,
    include_directories : [configinc],
    dependencies : [gstaudio_dep] + directsoundsink_dep,
    install : true,
    install_dir : plugins_install_dir)
  pkgconfig.generate(gstdirectsoundsink, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstdirectsoundsink]
endif
