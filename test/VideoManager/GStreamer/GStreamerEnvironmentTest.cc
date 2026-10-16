#include "GStreamerEnvironmentTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
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

    QVERIFY(GStreamer::Environment::usePrivateRegistry(QString()).isEmpty());
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY"));

    const QString cacheDir = QDir::cleanPath(root.filePath(QStringLiteral("cache/QGroundControl")));
    guint major = 0;
    guint minor = 0;
    guint micro = 0;
    guint nano = 0;
    gst_version(&major, &minor, &micro, &nano);
    const QString expected = QDir(cacheDir).filePath(QStringLiteral("gstreamer-%1.%2.%3-%4.bin")
                                                         .arg(major)
                                                         .arg(minor)
                                                         .arg(micro)
                                                         .arg(QSysInfo::buildCpuArchitecture()));

    QCOMPARE(GStreamer::Environment::usePrivateRegistry(cacheDir), expected);
    QCOMPARE(qEnvironmentVariable("GST_REGISTRY"), expected);
    QVERIFY(QFileInfo(cacheDir).isDir());
    QVERIFY(!qEnvironmentVariableIsSet("GST_REGISTRY_1_0"));
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
    QVERIFY(GStreamer::Environment::usePrivateRegistry(cacheDir).isEmpty());
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

#endif

UT_REGISTER_TEST(GStreamerEnvironmentTest, TestLabel::Unit)
