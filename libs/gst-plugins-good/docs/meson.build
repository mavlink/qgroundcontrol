build_hotdoc = false

if meson.is_cross_build()
    if get_option('doc').enabled()
        error('Documentation enabled but building the doc while cross building is not supported yet.')
    endif

    message('Documentation not built as building it while cross building is not supported yet.')
    subdir_done()
endif

required_hotdoc_extensions = ['gi-extension', 'gst-extension']
if gst_dep.type_name() == 'internal'
    gst_proj = subproject('gstreamer')
    plugins_cache_generator = gst_proj.get_variable('plugins_cache_generator')
else
    plugins_cache_generator = find_program(join_paths(gst_dep.get_pkgconfig_variable('libexecdir'), 'gstreamer-' + api_version, 'gst-plugins-doc-cache-generator'),
        required: false)
endif

plugins_cache = join_paths(meson.current_source_dir(), 'gst_plugins_cache.json')
if plugins.length() == 0
    message('All good plugins have been disabled')
elif plugins_cache_generator.found()
    plugins_doc_dep = custom_target('good-plugins-doc-cache',
        command: [plugins_cache_generator, plugins_cache, '@OUTPUT@', '@INPUT@'],
        input: plugins,
        output: 'gst_plugins_cache.json',
    )
else
    warning('GStreamer plugin inspector for documentation not found, can\'t update the cache')
endif

hotdoc_p = find_program('hotdoc', required: get_option('doc'))
if not hotdoc_p.found()
    message('Hotdoc not found, not building the documentation')
    subdir_done()
endif

hotdoc = import('hotdoc')
foreach extension: required_hotdoc_extensions
    if not hotdoc.has_extensions(extension)
        if get_option('doc').enabled()
            error('Documentation enabled but @0@ missing'.format(extension))
        endif

        message('@0@ extension not found, not building documentation'.format(extension))
        subdir_done()
    endif
endforeach

docconf = configuration_data()
docconf.set('GST_API_VERSION', api_version)
configure_file(input : 'gst_api_version.in',
               output : 'gst_api_version.md',
               configuration : docconf)
libs_doc = []
plugins_doc = []
excludes = []
build_hotdoc = true
foreach f: ['gstgdkpixbufplugin.c']
    excludes += [join_paths(meson.current_source_dir(), '..', 'ext/gdk_pixbuf/', f)]
endforeach

list_plugin_res = run_command(python3, '-c',
'''
import sys
import json

with open("@0@") as f:
    print(':'.join(json.load(f).keys()), end='')
'''.format(plugins_cache))

assert(list_plugin_res.returncode() == 0,
  'Could not list plugins from @0@'.format(plugins_cache))

foreach plugin_name: list_plugin_res.stdout().split(':')
    plugins_doc += [hotdoc.generate_doc(plugin_name,
    project_version: api_version,
      sitemap: 'sitemap.txt',
      index: 'index.md',
      gst_index: 'index.md',
      gst_smart_index: true,
      gst_c_sources: ['../sys/*/*.[ch]',
                      '../ext/*/*.[ch]',
                      '../gst/*/*.[ch]',
                      ],
      gst_c_source_filters: excludes,
      dependencies: [gst_dep, plugins],
      gst_order_generated_subpages: true,
      install: false,
      disable_incremental_build: true,
      gst_cache_file: plugins_cache,
      gst_plugin_name: plugin_name,
      include_paths: join_paths(meson.current_source_dir(), '..'),
      )]
endforeach
