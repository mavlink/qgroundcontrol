#pragma once

// STL - frequently used across codebase
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Qt Core - used in nearly every file
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtCore/QVector>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

// Qt Network - used in 30+ files
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

// Qt Quick - QQuickItem used in 30+ files
#include <QtQuick/QQuickItem>

// MAVLink - used in 400+ locations
#include "MAVLinkLib.h"