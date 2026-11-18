#include "LoggingQmlBindingTest.h"

#include <QtQml/QQmlComponent>

#include "LogEntry.h"
#include "LogManager.h"
#include "LogModel.h"
#include "LogTestHelpers.h"

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
        import QGroundControl.LogManager

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

void LoggingQmlBindingTest::_levelValues()
{
    // Enums now live on LogEntry directly — verify they're sane
    QCOMPARE(static_cast<int>(LogEntry::Debug), 0);
    QCOMPARE(static_cast<int>(LogEntry::Warning), 2);
    QCOMPARE(static_cast<int>(LogEntry::Critical), 3);
    QCOMPARE(static_cast<int>(LogEntry::Fatal), 4);
}

void LoggingQmlBindingTest::_modelReceivesEntries()
{
    auto* model = LogManager::instance()->model();
    const int before = model->totalCount();

    // QTest replaces LogManager's message handler while a test runs, so a logged message never reaches the model.
    model->enqueue(LogTestHelpers::makeEntry(QStringLiteral("test message for model")));
    QTRY_VERIFY_WITH_TIMEOUT(model->totalCount() > before, TestTimeout::shortMs());

    QQmlComponent component(_engine);
    component.setData(R"(
        import QtQuick
        import QGroundControl.LogManager

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
        import QGroundControl.LogManager

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
}

UT_REGISTER_TEST(LoggingQmlBindingTest, TestLabel::Unit, TestLabel::Utilities)
