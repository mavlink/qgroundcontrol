/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

#include "TerrainQueryInterface.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainQueryArduPilotLog)

class QGeoCoordinate;

class TerrainQueryArduPilot : public TerrainOnlineQuery
{
    Q_OBJECT

public:
    explicit TerrainQueryArduPilot(QObject *parent = nullptr);
    ~TerrainQueryArduPilot();

    void requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates) final;

private:
    void _sendQuery(const QUrl &path);
    void _parseZipFile(const QString &remoteFile, const QString &localFile, const QString &errorMsg);
    void _parseCoordinateData(const QString &localFile, const QByteArray &hgtData);
};
