#include "LabelledFactBrowseTest.h"

#include <memory>

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include "Fact.h"
#include "FactMetaData.h"

namespace {

constexpr int kContainerWidth = 300;

constexpr const char* kBrowseInContainerQml = R"(
import QtQuick
import QtQuick.Layouts
import QGroundControl.FactControls

ColumnLayout {
    id: root
    width: 300
    property var testFact

    LabelledFactBrowse {
        objectName: "browse"
        Layout.fillWidth: true
        label: "Path"
        fact: root.testFact
    }
}
)";

}  // namespace

void LabelledFactBrowseTest::_longPathStaysWithinContainer_test()
{
    Fact fact(0, QStringLiteral("SavePath"), FactMetaData::valueTypeString);
    fact.setRawValue(QString(120, QLatin1Char('x')));

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kBrowseInContainerQml, QUrl());
    const std::unique_ptr<QObject> container(
        component.createWithInitialProperties({{QStringLiteral("testFact"), QVariant::fromValue(&fact)}}));
    QVERIFY2(container, qPrintable(component.errorString()));

    const auto* const browse = container->findChild<QQuickItem*>(QStringLiteral("browse"));
    QVERIFY(browse);

    for (const QQuickItem* child : browse->childItems()) {
        QVERIFY2(child->x() + child->width() <= kContainerWidth,
                 qPrintable(QStringLiteral("a browse control child ends at %1, past the %2 px container")
                                .arg(child->x() + child->width())
                                .arg(kContainerWidth)));
    }
}

UT_REGISTER_TEST(LabelledFactBrowseTest, TestLabel::Unit)
