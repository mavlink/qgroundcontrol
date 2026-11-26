#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GStreamerEnvironment.h"

/// The checks run() makes for the boot test (--simple-boot-test), proving that the GStreamer runtime the
/// app ships works: its plugins load, every feature's elements exist, H.264 (and H.265, given a decoder) decodes, MP4
/// and MKV record, and GIO has TLS and, where the bundle ships libproxy, proxy support. Each failed check is logged
/// under Video.GStreamer.SelfTest, naming the element, plugin or file at fault.
namespace GStreamer::SelfTest {

/// A plugin as GStreamer's registry has it.
struct RegisteredPlugin
{
    QString name;
    QString filename;  ///< Empty for a statically registered plugin.
    bool blacklisted = false;
};

/// What is wrong with a bundled runtime's plugins, one message each: plugins GStreamer blacklisted (typically because a
/// library they link is missing), registered plugins from outside @p pluginDir or whose file the bundle doesn't ship,
/// and plugin files in @p pluginDir (@p pluginFiles) that the registry lacks. Static plugins are not checked.
QStringList bundledPluginIssues(const QList<RegisteredPlugin>& registered, const QString& pluginDir,
                                const QStringList& pluginFiles);

#if defined(Q_OS_WIN)
/// Why GStreamer for Windows won't use @p scanner, found the way it finds out before a scan
/// (gstpluginloader-win32.c): started without arguments, a working gst-plugin-scanner exits with 1 at once. Empty
/// when it does.
QString pluginScannerStartFailure(const QString& scanner);
#endif

/// Runs every check that applies to @p runtime, within about 15 s, and returns whether all passed. Needs an
/// initialized GStreamer and the GUI thread; restores any decoder rank it changes.
bool run(const Environment::BundledRuntime& runtime);

}  // namespace GStreamer::SelfTest
