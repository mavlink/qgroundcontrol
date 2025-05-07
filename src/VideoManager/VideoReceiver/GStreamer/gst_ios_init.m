#include <gst/gst.h>
#include <gio/gio.h>
#include <Foundation/Foundation.h>

/* Declaration of static plugins */
 @PLUGINS_DECLARATION@

/* Declaration of static gio modules */
 @G_IO_MODULES_DECLARE@

#ifdef GSTREAMER_INCLUDE_CA_CERTIFICATES
static void
gst_ios_load_certificates (const gchar *resources_dir)
{
  gchar *ca_certificates;

  ca_certificates = g_build_filename (resources_dir, "ssl", "certs", "ca-certificates.crt", NULL);
  g_setenv ("CA_CERTIFICATES", ca_certificates, TRUE);

  if (ca_certificates) {
    GTlsBackend *backend = g_tls_backend_get_default ();
    if (backend) {
      GTlsDatabase *db = g_tls_file_database_new (ca_certificates, NULL);
      if (db)
        g_tls_backend_set_default_database (backend, db);
    }
  }
  g_free (ca_certificates);
}

#endif

void
gst_ios_init (void)
{
  GstPluginFeature *plugin;
  GstRegistry *reg;
  NSString *resources = [[NSBundle mainBundle] resourcePath];
  NSString *tmp = NSTemporaryDirectory();
  NSString *cache = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Caches"];
  NSString *docs = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];

  const gchar *resources_dir = [resources UTF8String];
  const gchar *tmp_dir = [tmp UTF8String];
  const gchar *cache_dir = [cache UTF8String];
  const gchar *docs_dir = [docs UTF8String];

  g_setenv ("TMP", tmp_dir, TRUE);
  g_setenv ("TEMP", tmp_dir, TRUE);
  g_setenv ("TMPDIR", tmp_dir, TRUE);
  g_setenv ("XDG_RUNTIME_DIR", resources_dir, TRUE);
  g_setenv ("XDG_CACHE_HOME", cache_dir, TRUE);

  g_setenv ("HOME", docs_dir, TRUE);
  g_setenv ("XDG_DATA_DIRS", resources_dir, TRUE);
  g_setenv ("XDG_CONFIG_DIRS", resources_dir, TRUE);
  g_setenv ("XDG_CONFIG_HOME", cache_dir, TRUE);
  g_setenv ("XDG_DATA_HOME", resources_dir, TRUE);
  g_setenv ("FONTCONFIG_PATH", resources_dir, TRUE);

  @G_IO_MODULES_LOAD@

#ifdef GSTREAMER_INCLUDE_CA_CERTIFICATES
  gst_ios_load_certificates (resources_dir);
#endif

  gst_init (NULL, NULL);

  @PLUGINS_REGISTRATION@

  /* Lower the ranks of filesrc and giosrc so iosavassetsrc is
   * tried first in gst_element_make_from_uri() for file:// */
  reg = gst_registry_get();
  plugin = gst_registry_lookup_feature(reg, "filesrc");
  if (plugin)
    gst_plugin_feature_set_rank(plugin, GST_RANK_SECONDARY);
  plugin = gst_registry_lookup_feature(reg, "giosrc");
  if (plugin)
    gst_plugin_feature_set_rank(plugin, GST_RANK_SECONDARY-1);
}
