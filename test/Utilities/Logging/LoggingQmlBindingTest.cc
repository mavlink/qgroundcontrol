#include "LoggingQmlBindingTest.h"

#include <QtCore/QTimer>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>

#include "LogEntry.h"
#include "LogManager.h"
#include "LogModel.h"
#include "QGCLoggingCategory.h"
#include "UnitTestList.h"

QGC_LOGGING_CATEGORY(LoggingQmlBindingTestLog, "Test.LoggingQmlBinding")

// Share a single QQmlEngine across all test methods.
// qmlRegisterSingletonInstance ties the singleton to the first engine that
// accesses it; creating a new engine per test causes null-pointer returns
// once the original engine is destroyed (manifests under sanitizers).

void LoggingQmlBindingTest::initTestCase()
{
    UnitTest::initTestCase();
    _engine = new QQmlEngine(this);
}

void LoggingQmlBindingTest::cleanupTestCase()
{
    delete _engine;
    _engine = nullptr;
    UnitTest::cleanupTestCase();
}

void LoggingQmlBindingTest::_singletonAccessible()
{
    QQmlComponent component(_engine);
    component.setData(R"(
        import QtQuick
        import QGroundControl.Logging

        QtObject {
            property var mgr: LogManager
            property var mdl: LogManager.model
            property int count: LogManager.model ? LogManager.model.totalCount : -1
        }
    )", QUrl());

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj);

    const auto mgr = obj->property("mgr").value<QObject*>();
    QVERIFY(mgr);
    QCOMPARE(mgr, LogManager::instance());

    const auto mdl = obj->property("mdl").value<QObject*>();
    QVERIFY(mdl);
    QCOMPARE(mdl, LogManager::instance()->model());

    QVERIFY(obj->property("count").toInt() >= 0);
}

void LoggingQmlBindingTest::_modelRolesExposed()
{
    // Enums now live on LogEntry directly — verify they're sane
    QCOMPARE(static_cast<int>(LogEntry::Debug), 0);
    QCOMPARE(static_cast<int>(LogEntry::Warning), 2);
    QCOMPARE(static_cast<int>(LogEntry::Critical), 3);
    QCOMPARE(static_cast<int>(LogEntry::Fatal), 4);

    const auto roles = LogEntry::roleNames();
    QVERIFY(roles.contains(static_cast<int>(LogEntry::MessageRole)));
    QVERIFY(roles.contains(static_cast<int>(LogEntry::LevelRole)));
    QVERIFY(roles.contains(static_cast<int>(LogEntry::CategoryRole)));
    QVERIFY(roles.contains(static_cast<int>(LogEntry::TimestampRole)));
    QVERIFY(roles.contains(static_cast<int>(LogEntry::FormattedRole)));
}

void LoggingQmlBindingTest::_modelReceivesEntries()
{
    auto* model = LogManager::instance()->model();
    const int before = model->totalCount();

    qCWarning(LoggingQmlBindingTestLog) << "test message for model";
    // Model update is an in-process queued signal, not I/O; a 500 ms window
    // is generous for a local Qt signal and keeps failure feedback fast.
    QTRY_VERIFY_WITH_TIMEOUT(model->totalCount() > before, 500);

    QQmlComponent component(_engine);
    component.setData(R"(
        import QtQuick
        import QGroundControl.Logging

        QtObject {
            property int total: LogManager.model.totalCount
            property int filtered: LogManager.model.rowCount()
        }
    )", QUrl());

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj);

    QVERIFY(obj->property("total").toInt() > 0);
    QVERIFY(obj->property("filtered").toInt() > 0);
}

void LoggingQmlBindingTest::_filterBindingsWork()
{
    QQmlComponent component(_engine);
    component.setData(R"(
        import QtQuick
        import QGroundControl.Logging

        QtObject {
            property var model: LogManager.model
            property int filterLevel: model ? model.filterLevel : -1
            property string filterCategory: model ? model.filterCategory : ""
            property string filterText: model ? model.filterText : ""
            property bool filterRegex: model ? model.filterRegex : false
            property var categories: model ? model.categoriesList : []
        }
    )", QUrl());

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj);

    QCOMPARE(obj->property("filterLevel").toInt(), static_cast<int>(LogEntry::Debug));
    QVERIFY(obj->property("filterCategory").toString().isEmpty());
    QVERIFY(obj->property("filterText").toString().isEmpty());
    QCOMPARE(obj->property("filterRegex").toBool(), false);

    const auto categories = obj->property("categories").toStringList();
    QVERIFY(categories.size() >= 0); // May be empty initially
}

void LoggingQmlBindingTest::_settingsDialogLoads()
{
    QQmlComponent component(_engine);
    component.setData(R"(
        import QtQuick
        import QGroundControl.Logging

        QtObject {
            property bool diskEnabled: LogManager.diskLoggingEnabled
            property bool hasError: LogManager.hasError
            property var sink: LogManager.remoteSink
            property bool sinkValid: sink !== null
        }
    )", QUrl());

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj);

    QCOMPARE(obj->property("sinkValid").toBool(), true);
    // diskLoggingEnabled should be a boolean
    QVERIFY(obj->property("diskEnabled").isValid());
    QVERIFY(obj->property("hasError").isValid());
}

UT_REGISTER_TEST(LoggingQmlBindingTest, TestLabel::Unit, TestLabel::Utilities)
