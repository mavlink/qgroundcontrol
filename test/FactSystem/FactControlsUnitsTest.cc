#include "FactControlsUnitsTest.h"

#include <memory>

#include <QtCore/QScopeGuard>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtQml/QQmlProperty>

#include "Fact.h"
#include "FactMetaData.h"
#include "SettingsManager.h"
#include "UnitsSettings.h"

namespace {

constexpr const char* kFactTextFieldQml = R"(
import QtQuick
import QGroundControl.FactControls

FactTextField {}
)";

constexpr const char* kFactSliderQml = R"(
import QtQuick
import QGroundControl.Controls

FactSlider {}
)";

std::unique_ptr<QObject> createControl(QQmlEngine& engine, const QByteArray& qml, const QVariantMap& properties,
                                       QString& error)
{
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    std::unique_ptr<QObject> control(component.createWithInitialProperties(properties));
    error = component.errorString();
    return control;
}

}  // namespace

void FactControlsUnitsTest::_factTextFieldFocusOutKeepsRawValue_test()
{
    Fact* const verticalUnitsFact = SettingsManager::instance()->unitsSettings()->verticalDistanceUnits();
    const QVariant savedUnits = verticalUnitsFact->rawValue();
    const auto restoreUnits =
        qScopeGuard([verticalUnitsFact, savedUnits] { verticalUnitsFact->setRawValue(savedUnits); });
    verticalUnitsFact->setRawValue(UnitsSettings::VerticalDistanceUnitsMeters);

    Fact fact(0, QStringLiteral("Altitude"), FactMetaData::valueTypeDouble);
    fact.metaData()->setRawUnits(QStringLiteral("vertical m"));
    fact.setRawValue(50.0);

    QQmlEngine engine;
    QString error;
    const QVariantMap properties = {{QStringLiteral("fact"), QVariant::fromValue(&fact)}};
    const auto textField = createControl(engine, kFactTextFieldQml, properties, error);
    QVERIFY2(textField, qPrintable(error));

    verticalUnitsFact->setRawValue(UnitsSettings::VerticalDistanceUnitsFeet);
    QCOMPARE(textField->property("text").toString(), fact.cookedValueString());

    // Focus loss emits editingFinished even though the user never edited the rounded display text
    QVERIFY(QMetaObject::invokeMethod(textField.get(), "editingFinished"));

    QCOMPARE(fact.rawValue().toDouble(), 50.0);
}

void FactControlsUnitsTest::_factSliderTracksUnitsChangeAfterDrag_test()
{
    Fact* const horizontalUnitsFact = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    const QVariant savedUnits = horizontalUnitsFact->rawValue();
    const auto restoreUnits =
        qScopeGuard([horizontalUnitsFact, savedUnits] { horizontalUnitsFact->setRawValue(savedUnits); });
    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsMeters);

    Fact fact(0, QStringLiteral("Distance"), FactMetaData::valueTypeDouble);
    fact.metaData()->setRawUnits(QStringLiteral("m"));
    fact.metaData()->setRawMin(0.0);
    fact.metaData()->setRawMax(100.0);
    fact.setRawValue(50.0);

    QQmlEngine engine;
    QString error;
    const QVariantMap properties = {
        {QStringLiteral("fact"), QVariant::fromValue(&fact)},
        {QStringLiteral("majorTickStepSize"), 10},
    };
    const auto slider = createControl(engine, kFactSliderQml, properties, error);
    QVERIFY2(slider, qPrintable(error));
    QCOMPARE(slider->property("value").toDouble(), 50.0);

    // A user drag assigns value directly, which replaces the value binding
    QVERIFY(QQmlProperty::write(slider.get(), QStringLiteral("value"), 40.0));
    QTRY_COMPARE_WITH_TIMEOUT(fact.rawValue().toDouble(), 40.0, TestTimeout::mediumMs());

    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsFeet);

    QCOMPARE_FUZZY(slider->property("value").toDouble(), fact.cookedValue().toDouble(), 1e-9);
    QQmlExpression timerRunning(qmlContext(slider.get()), slider.get(), QStringLiteral("updateTimer.running"));
    QCOMPARE(timerRunning.evaluate().toBool(), false);
    QCOMPARE(fact.rawValue().toDouble(), 40.0);
}

void FactControlsUnitsTest::_factSliderRepositionsOnUnitsChange_test()
{
    Fact* const horizontalUnitsFact = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    const QVariant savedUnits = horizontalUnitsFact->rawValue();
    const auto restoreUnits =
        qScopeGuard([horizontalUnitsFact, savedUnits] { horizontalUnitsFact->setRawValue(savedUnits); });
    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsMeters);

    Fact fact(0, QStringLiteral("Distance"), FactMetaData::valueTypeDouble);
    fact.metaData()->setRawUnits(QStringLiteral("m"));
    fact.metaData()->setRawMin(0.0);
    fact.metaData()->setRawMax(100.0);
    fact.setRawValue(50.0);

    QQmlEngine engine;
    QString error;
    const QVariantMap properties = {
        {QStringLiteral("fact"), QVariant::fromValue(&fact)},
        {QStringLiteral("majorTickStepSize"), 10},
        {QStringLiteral("width"), 300},
    };
    const auto slider = createControl(engine, kFactSliderQml, properties, error);
    QVERIFY2(slider, qPrintable(error));

    // The value indicator is fixed at the center; the tick strip moves underneath it
    QObject* const background = slider->property("background").value<QObject*>();
    QVERIFY(background);
    QQmlExpression stripX(qmlContext(background), background, QStringLiteral("sliderContainer.x"));
    QQmlExpression expectedStripX(qmlContext(slider.get()), slider.get(), QStringLiteral("_valueToSliderXPos(value)"));
    const double initialStripX = stripX.evaluate().toDouble();
    QCOMPARE_FUZZY(initialStripX, expectedStripX.evaluate().toDouble(), 1e-6);

    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsFeet);

    QVERIFY(!qFuzzyCompare(expectedStripX.evaluate().toDouble(), initialStripX));
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(stripX.evaluate().toDouble() - expectedStripX.evaluate().toDouble()) < 1e-6,
                             TestTimeout::mediumMs());
}

UT_REGISTER_TEST(FactControlsUnitsTest, TestLabel::Unit)
