#pragma once

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QTest>

#include "ColoredSvgImageProvider.h"
#include "UnitTest.h"

namespace GpsTestHelpers {

/// A QML file in the source tree, for components tested from their source rather than the compiled module.
inline QUrl sourceQmlUrl(const QString& pathFromSrc)
{
    return QUrl::fromLocalFile(
        QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath(QStringLiteral("../../src/") + pathFromSrc));
}

/// Engine with the application QML modules and the colored SVG images QGC controls use.
class QmlEngine : public QQmlEngine
{
public:
    QmlEngine()
    {
        addImportPath(QStringLiteral("qrc:/qml"));
        addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());
    }

    /// Loads and creates a component; null on failure, with the reason in lastError().
    std::unique_ptr<QObject> create(const QUrl& url, const QVariantMap& properties = {})
    {
        QQmlComponent component(this, url);
        return _create(component, properties);
    }

    /// Creates inline QML; null on failure, with the reason in lastError().
    std::unique_ptr<QObject> create(const QByteArray& qml, const QVariantMap& properties = {})
    {
        QQmlComponent component(this);
        component.setData(qml, QUrl());
        return _create(component, properties);
    }

    QString lastError() const { return _lastError; }

private:
    std::unique_ptr<QObject> _create(QQmlComponent& component, const QVariantMap& properties)
    {
        if (!QTest::qWaitFor([&component]() { return !component.isLoading(); }, TestTimeout::mediumMs())) {
            _lastError = QStringLiteral("Component did not finish loading");
            return {};
        }
        std::unique_ptr<QObject> object(component.createWithInitialProperties(properties));
        _lastError = component.errorString();
        return object;
    }

    QString _lastError;
};

}  // namespace GpsTestHelpers
