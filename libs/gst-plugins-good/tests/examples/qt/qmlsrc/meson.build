sources = [
  'main.cpp',
]

if have_cxx and build_gstgl and gstgl_dep.found()
  qt5_mod = import('qt5')
  qt5qml_deps = dependency('qt5', modules : ['Core', 'Gui', 'Widgets', 'Qml', 'Quick'],
                           required: get_option('examples'))
  # FIXME Add a way to get that information out of the qt5 module
  moc = find_program('moc-qt5', 'moc', required : get_option('examples'))
  if qt5qml_deps.found() and moc.found()
    qt_preprocessed = qt5_mod.preprocess(qresources : 'qmlsrc.qrc')
    executable('qmlsrc', sources, qt_preprocessed,
        dependencies : [gst_dep, qt5qml_deps],
        c_args : gst_plugins_good_args,
        include_directories : [configinc],
        install: false)
  endif
endif
