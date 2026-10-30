#include "GStreamerEnvironmentTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <vector>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSysInfo>
#include <QtCore/QTemporaryDir>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamerEnvironment.h"

namespace {

/// Saves the variables GStreamer::Environment reads or sets, unsets them, and restores them on destruction. PATH, which
/// a Windows bundle extends, is only saved.
class ManagedEnvGuard
{
public:
    ManagedEnvGuard()
    {
        static constexpr const char* kVars[] = {
            "APPDIR",
            "APPIMAGE",
            "GIO_EXTRA_MODULES",
            "GIO_MODULE_DIR",
            "GIO_USE_VFS",
            "GST_PLUGIN_PATH",
            "GST_PLUGIN_PATH_1_0",
            "GST_PLUGIN_SCANNER",
            "GST_PLUGIN_SCANNER_1_0",
            "GST_PLUGIN_SYSTEM_PATH",
            "GST_PLUGIN_SYSTEM_PATH_1_0",
            "GST_PTP_HELPER",
            "GST_PTP_HELPER_1_0",
            "GST_REGISTRY",
            "GST_REGISTRY_1_0",
            "GST_REGISTRY_FORK",
            "GST_REGISTRY_REUSE_PLUGIN_SCANNER",
            "PYTHONNOUSERSITE",
            "PYTHONPATH",
        };
        _saved.reserve(std::size(kVars) + 1);
        for (const char* name : kVars) {
            _saved.emplace_back(name);
            qunsetenv(name);
        }
        _saved.emplace_back("PATH");
    }

private:
    std::vector<TestFixtures::EnvVarFixture> _saved;
};

bool setModificationTime(const QString& path, const QDateTime& time)
{
    QFile file(path);
    return file.open(QIODevice::ReadWrite) && file.setFileTime(time, QFileDevice::FileModificationTime);
}

}  // namespace

void GStreamerEnvironmentTest::_testPrivateRegistry()
{
    const ManagedEnvGuard guard;
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString stablePlugins = root.filePath(QStringLiteral("stable/lib/gstreamer-1.0"));
    const QString dailyPlugins = root.filePath(QStringLiteral("daily/lib/gstreamer-1.0"));

    QVERIFY(GStreamer::Environment::usePrivateRegistry(QString(), stablePlugins).isEmpty());
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY"));

    const QString cacheDir = QDir::cleanPath(root.filePath(QStringLiteral("cache/QGroundControl")));
    guint major = 0;
    guint minor = 0;
    guint micro = 0;
    guint nano = 0;
    gst_version(&major, &minor, &micro, &nano);
    const QRegularExpression expectedName(QStringLiteral("^gstreamer-%1\\.%2\\.%3-%4-[0-9a-f]{8}\\.bin$")
                                              .arg(major)
                                              .arg(minor)
                                              .arg(micro)
                                              .arg(QRegularExpression::escape(QSysInfo::buildCpuArchitecture())));

    const QString stable = GStreamer::Environment::usePrivateRegistry(cacheDir, stablePlugins);
    QCOMPARE(qEnvironmentVariable("GST_REGISTRY"), stable);
    QCOMPARE(QFileInfo(stable).absolutePath(), cacheDir);
    QVERIFY2(expectedName.match(QFileInfo(stable).fileName()).hasMatch(), qPrintable(stable));
    QVERIFY(QFileInfo(cacheDir).isDir());
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY_1_0"));

    // Two installs of one GStreamer version keep separate registries; one install keeps its own across launches.
    qunsetenv("GST_REGISTRY");
    QCOMPARE(GStreamer::Environment::usePrivateRegistry(cacheDir, stablePlugins), stable);
    qunsetenv("GST_REGISTRY");
    const QString daily = GStreamer::Environment::usePrivateRegistry(cacheDir, dailyPlugins);
    QVERIFY2(expectedName.match(QFileInfo(daily).fileName()).hasMatch(), qPrintable(daily));
    QVERIFY(daily != stable);
}

void GStreamerEnvironmentTest::_testPrivateRegistryStableInstall_data()
{
    QTest::addColumn<QString>("appImage");
    QTest::addColumn<QString>("firstMount");
    QTest::addColumn<QString>("firstPath");
    QTest::addColumn<QString>("secondMount");
    QTest::addColumn<QString>("secondPath");
    QTest::addColumn<bool>("shared");

    const QString plugins = QStringLiteral("/usr/lib/gstreamer-1.0");
    QTest::newRow("AppImage mounted again")
        << QStringLiteral("/opt/QGroundControl.AppImage") << QStringLiteral("/tmp/.mount_QGrounA")
        << QStringLiteral("/tmp/.mount_QGrounA") + plugins << QStringLiteral("/tmp/.mount_QGrounB")
        << QStringLiteral("/tmp/.mount_QGrounB") + plugins << true;
    QTest::newRow("AppImage environment inherited by another install")
        << QStringLiteral("/opt/Other.AppImage") << QStringLiteral("/tmp/.mount_OtherA")
        << QStringLiteral("/opt/qgc-stable/lib/gstreamer-1.0") << QStringLiteral("/tmp/.mount_OtherA")
        << QStringLiteral("/opt/qgc-daily/lib/gstreamer-1.0") << false;
    const QString translocated = QStringLiteral(
        "/private/var/folders/ab/cd/T/AppTranslocation/%1/d/QGroundControl.app/Contents/lib/gstreamer-1.0");
    QTest::newRow("macOS app translocated again")
        << QString() << QString() << translocated.arg(QStringLiteral("0A1B2C3D-0000-4000-8000-000000000001"))
        << QString() << translocated.arg(QStringLiteral("0A1B2C3D-0000-4000-8000-000000000002")) << true;
}

void GStreamerEnvironmentTest::_testPrivateRegistryStableInstall()
{
    QFETCH(QString, appImage);
    QFETCH(QString, firstMount);
    QFETCH(QString, firstPath);
    QFETCH(QString, secondMount);
    QFETCH(QString, secondPath);
    QFETCH(bool, shared);

    const ManagedEnvGuard guard;
    QTemporaryDir cacheDir;
    QVERIFY(cacheDir.isValid());
    const auto registryFor = [&](const QString& mount, const QString& path) {
        qunsetenv("GST_REGISTRY");
        if (!appImage.isEmpty()) {
            qputenv("APPIMAGE", appImage.toUtf8());
            qputenv("APPDIR", mount.toUtf8());
        }
        return GStreamer::Environment::usePrivateRegistry(cacheDir.path(), path);
    };

    const QString first = registryFor(firstMount, firstPath);
    const QString second = registryFor(secondMount, secondPath);
    QVERIFY(!first.isEmpty());
    QCOMPARE(first == second, shared);
}

void GStreamerEnvironmentTest::_testPrivateRegistryPrunesUnusedRegistries()
{
    const ManagedEnvGuard guard;
    QTemporaryDir cacheDir;
    QVERIFY(cacheDir.isValid());
    const QString plugins = cacheDir.filePath(QStringLiteral("install/lib/gstreamer-1.0"));
    const QString registry = GStreamer::Environment::usePrivateRegistry(cacheDir.path(), plugins);
    QVERIFY(!registry.isEmpty());

    const QDateTime unusedTime = QDateTime::currentDateTimeUtc().addDays(-31);
    const QString unused = cacheDir.filePath(QStringLiteral("gstreamer-1.20.0-x86_64.bin"));
    const QString recent = cacheDir.filePath(QStringLiteral("gstreamer-1.28.4-x86_64-0123abcd.bin"));
    const QString unrelated = cacheDir.filePath(QStringLiteral("other.bin"));
    for (const QString& path : {registry, unused, unrelated}) {
        QVERIFY2(setModificationTime(path, unusedTime), qPrintable(path));
    }
    QVERIFY(setModificationTime(recent, QDateTime::currentDateTimeUtc()));

    qunsetenv("GST_REGISTRY");
    QCOMPARE(GStreamer::Environment::usePrivateRegistry(cacheDir.path(), plugins), registry);
    QVERIFY(!QFileInfo::exists(unused));
    QVERIFY(QFileInfo::exists(recent));
    QVERIFY(QFileInfo::exists(unrelated));
    // An install's own registry, which GStreamer leaves alone while the plugins are unchanged, counts as used.
    QVERIFY(QFileInfo::exists(registry));
    QVERIFY(QFileInfo(registry).lastModified() > unusedTime.addDays(1));
}

void GStreamerEnvironmentTest::_testPrivateRegistryKeepsUserRegistry_data()
{
    QTest::addColumn<QByteArray>("variable");
    QTest::newRow("GST_REGISTRY") << QByteArrayLiteral("GST_REGISTRY");
    QTest::newRow("GST_REGISTRY_1_0") << QByteArrayLiteral("GST_REGISTRY_1_0");
}

void GStreamerEnvironmentTest::_testPrivateRegistryKeepsUserRegistry()
{
    QFETCH(QByteArray, variable);
    const ManagedEnvGuard guard;
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString userRegistry = root.filePath(QStringLiteral("user.bin"));
    qputenv(variable.constData(), userRegistry.toUtf8());

    const QString cacheDir = root.filePath(QStringLiteral("cache"));
    QVERIFY(GStreamer::Environment::usePrivateRegistry(cacheDir, root.filePath(QStringLiteral("plugins"))).isEmpty());
    QCOMPARE(qEnvironmentVariable(variable.constData()), userRegistry);
    if (variable != QByteArrayLiteral("GST_REGISTRY")) {
        QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY"));
    }
    QVERIFY(!QFileInfo::exists(cacheDir));
}

void GStreamerEnvironmentTest::_testSystemGStreamerKeepsDefaultRegistry()
{
#ifdef Q_OS_LINUX
    if (QFileInfo::exists(
            QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../lib/gstreamer-1.0")))) {
        QSKIP("The test binary runs from a bundle with its own GStreamer plugins");
    }
    const ManagedEnvGuard guard;
    QVERIFY(GStreamer::Environment::prepareEnvironment().ok);
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY"));
#else
    QSKIP("Only Linux builds use a system GStreamer");
#endif
}

void GStreamerEnvironmentTest::_testBundledRuntime()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(QGC_GST_SYSTEM_PLUGINS)
    QSKIP("This build never uses a bundled GStreamer runtime");
#else
    const ManagedEnvGuard guard;
    qputenv("PYTHONPATH", QByteArrayLiteral("/qgc-test/python"));

    // An app in <root>/bin with its runtime in <root>/lib and <root>/libexec, as the desktop installs lay it out.
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString appDir = root.filePath(QStringLiteral("bin"));
    const QString pluginDir = root.filePath(QStringLiteral("lib/gstreamer-1.0"));
#ifdef Q_OS_WIN
    const QString scanner = root.filePath(QStringLiteral("libexec/gstreamer-1.0/gst-plugin-scanner.exe"));
#else
    const QString scanner = root.filePath(QStringLiteral("libexec/gstreamer-1.0/gst-plugin-scanner"));
#endif
    for (const QString& dir : {appDir, pluginDir, QFileInfo(scanner).absolutePath()}) {
        QVERIFY(QDir().mkpath(dir));
    }
    QFile scannerFile(scanner);
    QVERIFY(scannerFile.open(QIODevice::WriteOnly));
    scannerFile.close();
    QVERIFY(scannerFile.setPermissions(scannerFile.permissions() | QFileDevice::ExeOwner));

    QVERIFY(GStreamer::Environment::prepareEnvironment(appDir).ok);

    QCOMPARE(qEnvironmentVariable("GST_PLUGIN_SYSTEM_PATH"), pluginDir);
    QCOMPARE(qEnvironmentVariable("GST_PLUGIN_SYSTEM_PATH_1_0"), pluginDir);
    QVERIFY(!qEnvironmentVariableIsSet("GST_PLUGIN_PATH"));
    QCOMPARE(qEnvironmentVariable("GST_PLUGIN_SCANNER"), scanner);
    QCOMPARE(qEnvironmentVariable("GST_PLUGIN_SCANNER_1_0"), scanner);
    // Desktop GStreamer scans plugins in its forked scanner, so a crashing plugin can't take QGC down, and reuses one
    // scanner for the whole scan.
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY_FORK"));
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY_REUSE_PLUGIN_SCANNER"));
    QVERIFY(qEnvironmentVariableIsSet("GST_REGISTRY"));
    QCOMPARE(qEnvironmentVariable("PYTHONPATH"), QStringLiteral("/qgc-test/python"));
    QVERIFY(!qEnvironmentVariableIsSet("PYTHONNOUSERSITE"));
#endif
}

#else

void GStreamerEnvironmentTest::_testPrivateRegistry()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerEnvironmentTest::_testPrivateRegistryStableInstall_data() {}

void GStreamerEnvironmentTest::_testPrivateRegistryStableInstall()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerEnvironmentTest::_testPrivateRegistryPrunesUnusedRegistries()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerEnvironmentTest::_testPrivateRegistryKeepsUserRegistry_data() {}

void GStreamerEnvironmentTest::_testPrivateRegistryKeepsUserRegistry()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerEnvironmentTest::_testSystemGStreamerKeepsDefaultRegistry()
{
    QSKIP("GStreamer not enabled");
}

void GStreamerEnvironmentTest::_testBundledRuntime()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GStreamerEnvironmentTest, TestLabel::Unit)
