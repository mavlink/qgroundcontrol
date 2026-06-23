#pragma once

// STL
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

// Qt Core - fundamentals
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtGlobal>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>

// Qt Core - JSON
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

// Qt Core - utilities
#include <QtCore/QApplicationStatic>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>

// Qt Positioning - used in 50+ files
#include <QtPositioning/QGeoCoordinate>

// Qt Qml/Quick - QML integration macros used in ~130 headers.
// Note: QQuickItem intentionally omitted — pulls full QtQuick (scene graph, GL) everywhere.
#include <QtQml/QQmlEngine>
#include <QtQmlIntegration/QtQmlIntegration>

// MAVLink - used in 400+ locations
#include "MAVLinkLib.h"
