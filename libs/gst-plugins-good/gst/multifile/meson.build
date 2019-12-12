multifile_sources = [
  'gstmultifilesink.c',
  'gstmultifilesrc.c',
  'gstmultifile.c',
  'gstsplitfilesrc.c',
  'gstsplitmuxpartreader.c',
  'gstsplitmuxsink.c',
  'gstsplitmuxsrc.c',
  'gstsplitutils.c',
  'patternspec.c',
]

gstmultifile = library('gstmultifile',
  multifile_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gstvideo_dep, gstbase_dep,
                  gstpbutils_dep, gio_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstmultifile, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstmultifile]

test_splitmuxpartreader_sources = [
  'test-splitmuxpartreader.c',
  'gstsplitmuxpartreader.c',
  'gstsplitmuxsrc.c',
  'gstsplitutils.c',
  'patternspec.c',
]

executable('test-splitmuxpartreader',
  test_splitmuxpartreader_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gsttag_dep, gstbase_dep],
  install : false,
)
