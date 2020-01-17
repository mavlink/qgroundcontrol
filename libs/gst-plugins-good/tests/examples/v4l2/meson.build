executable('camctrl', 'camctrl.c',
  dependencies: [gstcontroller_dep, gst_dep],
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  install: false)

executable('v4l2src-renegotiate', 'v4l2src-renegotiate.c',
  dependencies: [gst_dep],
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  install: false)
