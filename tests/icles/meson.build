tests = [
  ['equalizer-test'],
  ['test-accurate-seek', [gstaudio_dep, gstapp_dep]],
  ['test-segment-seeks'],
  ['videocrop-test'],
  ['videobox-test'],
  ['videocrop2-test'],
]

if gtk_dep.found()
  tests += [
    ['gdkpixbufsink-test', gtk_dep],
    ['gdkpixbufoverlay-test', [gstvideo_dep, gtk_dep]],
  ]
endif

if get_variable('have_oss4', false)
  tests += [['test-oss4']]
endif

if x11_dep.found()
  tests += [['ximagesrc-test']]
endif

foreach t : tests
  test_name = t.get(0)
  extra_deps = t.get(1, [])
  executable(test_name, test_name + '.c',
    dependencies: [gst_dep, gstbase_dep, libm, extra_deps],
    c_args : gst_plugins_good_args,
    include_directories : [configinc],
    install: false)
endforeach
