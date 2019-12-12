speex_sources = [
  'gstspeex.c',
  'gstspeexdec.c',
  'gstspeexenc.c',
]

speex_dep = dependency('speex', version : '>=1.1.6', required : get_option('speex'))
windows_args = []
if host_system == 'windows'
  windows_args = ['-DWIN32']
endif

if speex_dep.found()
  gstspeex = library('gstspeex',
    speex_sources,
    c_args : gst_plugins_good_args + ['-DGST_USE_UNSTABLE_API'] + windows_args,
    link_args : noseh_link_args,
    include_directories : [configinc],
    dependencies : [gstbase_dep, gsttag_dep, gstaudio_dep, speex_dep, libm],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstspeex, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstspeex]
endif
