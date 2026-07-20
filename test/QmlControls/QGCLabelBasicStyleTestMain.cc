#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QScopedPointer>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuickControls2/QQuickStyle>
#include <array>
#include <cstdlib>

int main(int argc, char* argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication app(argc, argv);

    if (app.arguments().size() != 2) {
        qCritical() << "Expected the QGCLabel.qml path";
        return EXIT_FAILURE;
    }

    QQmlEngine engine;
    const QUrl labelUrl = QUrl::fromLocalFile(QFileInfo(app.arguments().at(1)).absoluteFilePath());
    QQmlComponent component(&engine, labelUrl);
    QScopedPointer<QObject> label(component.create());

    if (!label) {
        qCritical().noquote() << "QML load failed:" << component.errorString();
        return EXIT_FAILURE;
    }

    if (QQuickStyle::name() != QStringLiteral("Basic")) {
        qCritical() << "Unexpected Qt Quick Controls style:" << QQuickStyle::name();
        return EXIT_FAILURE;
    }

    static constexpr std::array labelProperties = {"antialiasing", "color", "font", "linkColor", "text"};
    for (const char* propertyName : labelProperties) {
        if (label->metaObject()->indexOfProperty(propertyName) < 0) {
            qCritical() << "Missing Label property:" << propertyName;
            return EXIT_FAILURE;
        }
    }

    if (!label->property("antialiasing").toBool()) {
        qCritical() << "QGCLabel antialiasing default was not applied";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
