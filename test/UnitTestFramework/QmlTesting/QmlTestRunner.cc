#include "QmlTestRunner.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include "QmlTesting.h"

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
    // Qt Quick Test's quick_test_main() calls exit(), so it cannot run inside the
    // normal test harness. QML tests must be built and invoked as a separate
    // executable via CTest. This runner validates that the test files are present
    // and structurally valid, but actual QML execution happens out-of-process.
    const QString testDir = _testDirectory();
    if (testDir.isEmpty()) {
        QSKIP("QML test directory not found (quick_test_main requires a separate executable)");
        return;
    }

    const QDir dir(testDir);
    const QStringList testFiles = dir.entryList({"tst_*.qml"}, QDir::Files);
    if (testFiles.isEmpty()) {
        QSKIP("No QML test files found");
        return;
    }

    // Validate structure of discovered QML test files
    for (const QString& file : testFiles) {
        const QString path = dir.absoluteFilePath(file);
        QFile f(path);
        QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable("Cannot open: " + path));
        const QByteArray content = f.readAll();
        QVERIFY2(content.contains("TestCase"), qPrintable("Missing TestCase in: " + file));
        QVERIFY2(content.contains("function test_"), qPrintable("No test functions in: " + file));
    }
}
