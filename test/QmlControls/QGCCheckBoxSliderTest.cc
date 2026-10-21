#include "QGCCheckBoxSliderTest.h"

#include <memory>

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

namespace {

constexpr const char* kSliderQml = R"(
import QtQuick
import QGroundControl.Controls

QGCCheckBoxSlider {}
)";

const QString kLongText = QStringLiteral("Ignore loss while flying an autonomous mission over the horizon");

std::unique_ptr<QQuickItem> createSlider(QQmlEngine& engine, const QString& text, QString& error)
{
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kSliderQml, QUrl());
    std::unique_ptr<QObject> object(component.createWithInitialProperties({{QStringLiteral("text"), text}}));
    error = component.errorString();

    auto* const item = qobject_cast<QQuickItem*>(object.get());
    if (!item) {
        if (object) {
            error = QStringLiteral("Root object is not a QQuickItem");
        }
        return {};
    }
    (void) object.release();
    return std::unique_ptr<QQuickItem>(item);
}

QQuickItem* sliderLabel(const QQuickItem* slider)
{
    return slider->findChild<QQuickItem*>(QStringLiteral("checkBoxSlider_label"));
}

QQuickItem* sliderIndicator(const QQuickItem* slider)
{
    return slider->findChild<QQuickItem*>(QStringLiteral("checkBoxSlider_indicator"));
}

}  // namespace

void QGCCheckBoxSliderTest::_unconstrainedLabelStaysOnOneLine_test()
{
    QQmlEngine engine;
    QString error;
    const auto slider = createSlider(engine, kLongText, error);
    QVERIFY2(slider, qPrintable(error));

    QQuickItem* const label = sliderLabel(slider.get());
    QVERIFY(label);
    QCOMPARE(slider->width(), slider->implicitWidth());
    QCOMPARE(label->property("lineCount").toInt(), 1);
}

void QGCCheckBoxSliderTest::_narrowLabelWrapsBesideIndicator_test()
{
    QQmlEngine engine;
    QString error;
    const auto slider = createSlider(engine, kLongText, error);
    QVERIFY2(slider, qPrintable(error));

    QQuickItem* const label = sliderLabel(slider.get());
    QQuickItem* const indicator = sliderIndicator(slider.get());
    QVERIFY(label);
    QVERIFY(indicator);
    const qreal singleLineWidth = slider->implicitWidth();
    const qreal singleLineHeight = slider->implicitHeight();

    slider->setWidth(singleLineWidth / 3);

    QVERIFY(label->property("lineCount").toInt() > 1);
    QVERIFY(label->x() + label->width() <= indicator->x());
    QVERIFY(slider->implicitHeight() > singleLineHeight);
    QCOMPARE(slider->implicitWidth(), singleLineWidth);
}

void QGCCheckBoxSliderTest::_noTextIsIndicatorOnly_test()
{
    QQmlEngine engine;
    QString error;
    const auto slider = createSlider(engine, QString(), error);
    QVERIFY2(slider, qPrintable(error));

    QQuickItem* const label = sliderLabel(slider.get());
    QQuickItem* const indicator = sliderIndicator(slider.get());
    QVERIFY(label);
    QVERIFY(indicator);
    QVERIFY(!label->isVisible());
    QCOMPARE(slider->implicitWidth(), indicator->width());
}

UT_REGISTER_TEST(QGCCheckBoxSliderTest, TestLabel::Unit)
