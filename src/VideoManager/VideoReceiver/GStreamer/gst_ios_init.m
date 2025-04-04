#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "gst_ios_init.h"

G_BEGIN_DECLS

#define GST_G_IO_MODULE_DECLARE(name) \
    extern void G_PASTE(g_io_module_, G_PASTE(name, _load_static)) (void)

#define GST_G_IO_MODULE_LOAD(name) \
    G_PASTE(g_io_module_, G_PASTE(name, _load_static)) ()

G_END_DECLS

#define GST_IOS_GIO_MODULE_GNUTLS

#if defined(GST_IOS_GIO_MODULE_GNUTLS)
  #include <gio/gio.h>
  GST_G_IO_MODULE_DECLARE(gnutls);
#endif

void gst_ios_pre_init(void)
{
  NSString *resources = [[NSBundle mainBundle] resourcePath];
  NSString *tmp = NSTemporaryDirectory();
  NSString *cache = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Caches"];
  NSString *docs = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    
  const gchar *resources_dir = [resources UTF8String];
  const gchar *tmp_dir = [tmp UTF8String];
  const gchar *cache_dir = [cache UTF8String];
  const gchar *docs_dir = [docs UTF8String];
  gchar *ca_certificates;
    
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

  ca_certificates = g_build_filename (resources_dir, "ssl", "certs", "ca-certificates.crt", NULL);
  g_setenv ("CA_CERTIFICATES", ca_certificates, TRUE);
  g_free (ca_certificates);
}

void gst_ios_post_init(void)
{
  GstPluginFeature *plugin;
  GstRegistry *reg;
  /* Lower the ranks of filesrc and giosrc so iosavassetsrc is
   * tried first in gst_element_make_from_uri() for file:// */

#if defined(GST_IOS_GIO_MODULE_GNUTLS)
    GST_G_IO_MODULE_LOAD(gnutls);
#endif

  reg = gst_registry_get();
  plugin = gst_registry_lookup_feature(reg, "filesrc");
  if (plugin)
    gst_plugin_feature_set_rank(plugin, GST_RANK_SECONDARY);
  plugin = gst_registry_lookup_feature(reg, "giosrc");
  if (plugin)
    gst_plugin_feature_set_rank(plugin, GST_RANK_SECONDARY-1);
  if (!gst_registry_lookup_feature(reg, "vtdec_hw")) {
    /* Usually there is no vtdec_hw plugin on iOS - in that case
     * we are increasing vtdec rank since VideoToolbox on iOS
     * tries to use hardware implementation first */
    plugin = gst_registry_lookup_feature(reg, "vtdec");
    if (plugin)
      gst_plugin_feature_set_rank(plugin, GST_RANK_PRIMARY + 1);
    }
}
