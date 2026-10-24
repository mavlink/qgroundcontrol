#include "GStreamerEnvironmentTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <vector>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSysInfo>
#include <QtCore/QTemporaryDir>

#include "Fixtures/RAIIFixtures.h"
#include "GStreamerEnvironment.h"

namespace {

/// Saves GST_REGISTRY and GST_REGISTRY_1_0, unsets them, and restores them on destruction.
struct RegistryEnvGuard
{
    TestFixtures::EnvVarFixture registry{"GST_REGISTRY"};
    TestFixtures::EnvVarFixture registry10{"GST_REGISTRY_1_0"};

    RegistryEnvGuard()
    {
        qunsetenv("GST_REGISTRY");
        qunsetenv("GST_REGISTRY_1_0");
    }
};

}  // namespace

void GStreamerEnvironmentTest::_testPrivateRegistry()
{
    const RegistryEnvGuard guard;
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

void GStreamerEnvironmentTest::_testPrivateRegistryKeepsUserRegistry_data()
{
    QTest::addColumn<QByteArray>("variable");
    QTest::newRow("GST_REGISTRY") << QByteArrayLiteral("GST_REGISTRY");
    QTest::newRow("GST_REGISTRY_1_0") << QByteArrayLiteral("GST_REGISTRY_1_0");
}

void GStreamerEnvironmentTest::_testPrivateRegistryKeepsUserRegistry()
{
    QFETCH(QByteArray, variable);
    const RegistryEnvGuard guard;
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
    const RegistryEnvGuard guard;
    QVERIFY(GStreamer::Environment::prepareEnvironment().ok);
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY"));
#else
    QSKIP("Only Linux builds use a system GStreamer");
#endif
}

void GStreamerEnvironmentTest::_testBundledRuntime()
{
    static constexpr const char* kTouchedVars[] = {
        "GIO_EXTRA_MODULES",
        "GIO_MODULE_DIR",
        "GIO_USE_VFS",
        "GST_PTP_HELPER",
        "GST_PTP_HELPER_1_0",
        "GST_PLUGIN_PATH",
        "GST_PLUGIN_PATH_1_0",
        "GST_PLUGIN_SYSTEM_PATH",
        "GST_PLUGIN_SYSTEM_PATH_1_0",
        "GST_PLUGIN_SCANNER",
        "GST_PLUGIN_SCANNER_1_0",
        "GST_REGISTRY_FORK",
        "GST_REGISTRY_REUSE_PLUGIN_SCANNER",
        "PYTHONPATH",
        "PYTHONNOUSERSITE",
    };
    std::vector<TestFixtures::EnvVarFixture> restore;
    restore.reserve(std::size(kTouchedVars));
    for (const char* var : kTouchedVars) {
        restore.emplace_back(var);
        qunsetenv(var);
    }
    qputenv("GST_PLUGIN_PATH", QByteArrayLiteral("/qgc-test/stale-plugins"));
    qputenv("PYTHONPATH", QByteArrayLiteral("/qgc-test/python"));

    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString pluginDirs = root.filePath(QStringLiteral("lib/gstreamer-1.0"));
    QVERIFY(QDir().mkpath(pluginDirs));
    const QString scanner = root.filePath(QStringLiteral("gst-plugin-scanner"));
    QFile scannerFile(scanner);
    QVERIFY(scannerFile.open(QIODevice::WriteOnly));
    scannerFile.close();
    QVERIFY(scannerFile.setPermissions(scannerFile.permissions() | QFileDevice::ExeOwner));

    if (!GStreamer::Environment::useBundledRuntime(pluginDirs, QString(), scanner, QString())) {
        QSKIP("This build never uses a bundled GStreamer runtime");
    }

    QCOMPARE(qEnvironmentVariable("GST_PLUGIN_SYSTEM_PATH"), pluginDirs);
    QCOMPARE(qEnvironmentVariable("GST_PLUGIN_SYSTEM_PATH_1_0"), pluginDirs);
    QVERIFY(!qEnvironmentVariableIsSet("GST_PLUGIN_PATH"));
    QCOMPARE(qEnvironmentVariable("GST_PLUGIN_SCANNER"), scanner);
    QCOMPARE(qEnvironmentVariable("GST_PLUGIN_SCANNER_1_0"), scanner);
    // Desktop GStreamer scans plugins in its forked scanner, so a crashing plugin can't take QGC down, and reuses one
    // scanner for the whole scan.
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY_FORK"));
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY_REUSE_PLUGIN_SCANNER"));
    QCOMPARE(qEnvironmentVariable("PYTHONPATH"), QStringLiteral("/qgc-test/python"));
    QVERIFY(!qEnvironmentVariableIsSet("PYTHONNOUSERSITE"));
}

#else

void GStreamerEnvironmentTest::_testPrivateRegistry()
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
