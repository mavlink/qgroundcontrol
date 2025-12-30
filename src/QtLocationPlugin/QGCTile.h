/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>

struct QGCTile
{
    enum TileState {
        StatePending = 0,
        StateDownloading,
        StateError,
        StateComplete
    };

    int x = 0;
    int y = 0;
    int z = 0;
    quint64 tileSet = UINT64_MAX;
    QString hash;
    QString type = QStringLiteral("Invalid"); // TODO: int?
};
Q_DECLARE_METATYPE(QGCTile)
Q_DECLARE_METATYPE(QGCTile*)
Q_DECLARE_METATYPE(QList<QGCTile*>)
