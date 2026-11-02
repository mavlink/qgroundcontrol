#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GStreamerEnvironment.h"

/// The checks GStreamer::selfTest() runs for the boot test (--simple-boot-test), proving that the GStreamer runtime the
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
/// library they link is missing), registered plugins from outside @p pluginDirs or whose file the bundle doesn't ship,
/// and plugin files in @p pluginDirs (@p pluginFiles) that the registry lacks. Static plugins are not checked.
QStringList bundledPluginIssues(const QList<RegisteredPlugin>& registered, const QStringList& pluginDirs,
                                const QStringList& pluginFiles);

/// Runs every check that applies to @p runtime, within about 15 s, and returns whether all passed. Needs an
/// initialized GStreamer; restores any decoder rank it changes.
bool run(const Environment::BundledRuntime& runtime);

}  // namespace GStreamer::SelfTest
