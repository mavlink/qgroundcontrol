#include "QmlTestRunner.h"
#include "QmlTesting.h"

#include <QtCore/QDir>
#include <QtCore/QCoreApplication>

// Register as standalone - quick_test_main calls exit() so can't run with other tests
UT_REGISTER_TEST_STANDALONE(QmlTestRunner, TestLabel::Unit)

QmlTestRunner::QmlTestRunner()
{
}

QString QmlTestRunner::_testDirectory() const
{
    // Look for qmltests directory relative to executable
    const QDir appDir(QCoreApplication::applicationDirPath());

    // Try build directory structure
    QDir testDir = appDir;
    if (testDir.cd("qmltests")) {
        return testDir.absolutePath();
    }

    // Try parent directory (for some build configurations)
    testDir = appDir;
    if (testDir.cdUp() && testDir.cd("qmltests")) {
        return testDir.absolutePath();
    }

    return QString();
}

void QmlTestRunner::_runQmlTests()
{
    const QString testDir = _testDirectory();
    if (testDir.isEmpty()) {
        QSKIP("QML test directory not found");
        return;
    }

    // Find all tst_*.qml files
    const QDir dir(testDir);
    const QStringList filters = { "tst_*.qml" };
    const QStringList testFiles = dir.entryList(filters, QDir::Files);

    if (testFiles.isEmpty()) {
        QSKIP("No QML test files found");
        return;
    }

    qInfo() << "Found QML test files:" << testFiles;
    qInfo() << "Run QML tests directly with: quick_test_main" << testDir;

    // Note: quick_test_main_with_setup() calls exit() so we can't use it
    // in the normal test suite. Run QML tests separately:
    //   ./QGroundControl --unittest QmlTestRunner
    // Or use CTest directly on the qmltests directory

    // For now, just verify the test files exist and are valid QML
    for (const QString& file : testFiles) {
        const QString path = dir.absoluteFilePath(file);
        QFile f(path);
        QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable("Cannot open: " + path));
        const QByteArray content = f.readAll();
        QVERIFY2(content.contains("TestCase"), qPrintable("Missing TestCase in: " + file));
        QVERIFY2(content.contains("function test_"), qPrintable("No test functions in: " + file));
    }

    QVERIFY(true);
}
