#include "PreFlightCheckModelTest.h"

#include <memory>

#include <QtCore/QMetaObject>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>

namespace {

constexpr const char* kModelQml = R"(
import QtQuick
import QGroundControl.Controls

PreFlightCheckModel {
    PreFlightCheckGroup { objectName: "first" }
    PreFlightCheckGroup { objectName: "second" }
}
)";

}  // namespace

void PreFlightCheckModelTest::_groupLocks_data()
{
    QTest::addColumn<bool>("enforceOrder");
    QTest::addColumn<bool>("prerequisitesPassed");
    QTest::addColumn<bool>("firstEnabled");
    QTest::addColumn<bool>("secondEnabled");

    // Neither group has passed, so with ordering enforced only the first can be worked on
    QTest::newRow("ordered") << true << true << true << false;
    QTest::newRow("unordered") << false << true << true << true;
    // Unmet prerequisites lock every group, whether or not ordering is enforced
    QTest::newRow("ordered, prerequisites pending") << true << false << false << false;
    QTest::newRow("unordered, prerequisites pending") << false << false << false << false;
}

void PreFlightCheckModelTest::_groupLocks()
{
    QFETCH(bool, enforceOrder);
    QFETCH(bool, prerequisitesPassed);
    QFETCH(bool, firstEnabled);
    QFETCH(bool, secondEnabled);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kModelQml, QUrl());
    QVariantMap properties;
    properties.insert(QStringLiteral("enforceOrder"), enforceOrder);
    properties.insert(QStringLiteral("prerequisitesPassed"), prerequisitesPassed);
    const std::unique_ptr<QObject> model(component.createWithInitialProperties(properties));
    QVERIFY2(model, qPrintable(component.errorString()));

    QVERIFY(QMetaObject::invokeMethod(model.get(), "updateGroupLocks"));

    const QObject* const first = model->findChild<QObject*>(QStringLiteral("first"));
    const QObject* const second = model->findChild<QObject*>(QStringLiteral("second"));
    QVERIFY(first);
    QVERIFY(second);
    QCOMPARE(first->property("enabled").toBool(), firstEnabled);
    QCOMPARE(second->property("enabled").toBool(), secondEnabled);
}

UT_REGISTER_TEST(PreFlightCheckModelTest, TestLabel::Unit)
