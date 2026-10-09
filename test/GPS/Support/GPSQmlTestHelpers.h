#pragma once

#include <functional>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "ColoredSvgImageProvider.h"
#include "UnitTest.h"

namespace GPSTest {

/// A QML file in the source tree, for components tested from their source rather than the compiled module.
inline QUrl sourceQmlUrl(const QString& pathFromSrc)
{
    return QUrl::fromLocalFile(
        QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath(QStringLiteral("../../../src/") + pathFromSrc));
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

/// A window for QML items tested outside the main window, for assertions that need layout, polish or rendering.
/// Declare it before the engine that creates the items, so the window outlives them.
class QmlWindowFixture
{
public:
    explicit QmlWindowFixture(int width = 640, int height = 800) { _window.resize(width, height); }

    QmlWindowFixture(const QmlWindowFixture&) = delete;
    QmlWindowFixture& operator=(const QmlWindowFixture&) = delete;

    QQuickWindow* window() { return &_window; }

    /// The parent for items created inside the window, through their initial "parent" property.
    QQuickItem* contentItem() const { return _window.contentItem(); }

    /// Shows the window, holding @a item unless it already has a parent, and waits until the window is exposed.
    [[nodiscard]] bool show(QQuickItem* item = nullptr)
    {
        if (item && !item->parentItem()) {
            item->setParentItem(_window.contentItem());
        }
        _window.show();
        return QTest::qWaitForWindowExposed(&_window, TestTimeout::mediumMs());
    }

private:
    QQuickWindow _window;
};

/// Every item from @a root down, in tree order, that @a predicate accepts; includes Repeater delegates and hidden
/// items.
inline QList<QQuickItem*> findItems(QQuickItem* root, const std::function<bool(QQuickItem*)>& predicate)
{
    QList<QQuickItem*> found;
    if (!root) {
        return found;
    }
    if (predicate(root)) {
        found.append(root);
    }
    for (auto* child : root->childItems()) {
        found.append(findItems(child, predicate));
    }
    return found;
}

}  // namespace GPSTest
