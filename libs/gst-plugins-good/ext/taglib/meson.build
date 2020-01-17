taglib_sources = [
  'gstapev2mux.cc',
  'gstid3v2mux.cc',
  'gsttaglibplugin.c',
]

taglib_dep = dependency('taglib', version : '>= 1.5', required : get_option('taglib'))

if taglib_dep.found() and add_languages('cpp')
  extra_args = []
  cxx = meson.get_compiler('cpp')
  if cxx.has_argument('-fvisibility=hidden')
    extra_args += ['-fvisibility=hidden']
  endif
  if cxx.has_argument('-fno-strict-aliasing')
    extra_args += ['-fno-strict-aliasing']
  endif

  gsttaglib = library('gsttaglib',
    taglib_sources,
    c_args : gst_plugins_good_args + ['-DGST_USE_UNSTABLE_API'],
    cpp_args : gst_plugins_good_args + ['-DGST_USE_UNSTABLE_API'] + extra_args,
    include_directories : [configinc, libsinc],
    dependencies : [gsttag_dep, taglib_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gsttaglib, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gsttaglib]
endif
