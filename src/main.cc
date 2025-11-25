/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtWidgets/QApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtCore/QDir>
#include <QtCore/QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    QCoreApplication::setApplicationName(QStringLiteral(QGC_APP_NAME));
    QCoreApplication::setOrganizationName(QStringLiteral(QGC_ORG_NAME));
    QCoreApplication::setOrganizationDomain(QStringLiteral(QGC_ORG_DOMAIN));
    QCoreApplication::setApplicationVersion(QStringLiteral(QGC_APP_VERSION_STR));

    qDebug() << "Starting" << QGC_APP_NAME << QGC_APP_VERSION_STR;

    // Create QML engine
    QQmlApplicationEngine engine;

    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/qml/MainWindow.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qWarning() << "Failed to load QML file:" << url;
        return -1;
    }

    qDebug() << "QML loaded successfully";

    return app.exec();
}
