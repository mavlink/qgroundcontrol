/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifdef __cplusplus
#include <QtBluetooth/QBluetoothSocket>
#include <QtCharts/QAbstractSeries>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QCoreApplication>
#include <QtGui/QGuiApplication>
#include <QtLocation/QGeoServiceProvider>
#include <QtMultimedia/QMediaDevices>
#include <QtNetwork/QNetworkAccessManager>
#include <QtPositioning/QGeoCoordinate>
#include <QtQml/QQmlApplicationEngine>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick/QQuickItem>
#include <QtQuickControls2/QQuickStyle>
#include <QtSql/QSqlDatabase>
#include <QtTest/QSignalSpy>
#include <QtTextToSpeech/QTextToSpeech>
#include <QtWidgets/QApplication>
#include <QtXml/QDomDocument>
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
#include <QtSerialPort/QSerialPort>
#endif
#ifdef Q_OS_ANDROID
#include <QtCore/QJniEnvironment>
#endif
#ifdef QT_DEBUG
#include <QtCore/QDebug>
#endif
#endif
