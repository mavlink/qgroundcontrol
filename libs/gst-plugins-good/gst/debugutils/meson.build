gstnavigationtest = library('gstnavigationtest',
  'gstnavigationtest.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstvideo_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstnavigationtest, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstnavigationtest]

gstdebug = library('gstdebug',
  'gstdebug.c',
  'breakmydata.c',
  'gstcapssetter.c',
  'gstnavseek.c',
  'gstpushfilesrc.c',
  'gsttaginject.c',
  'rndbuffersize.c',
  'progressreport.c',
  'tests.c',
  'cpureport.c',
  'testplugin.c',
  c_args: gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstdebug, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstdebug]
