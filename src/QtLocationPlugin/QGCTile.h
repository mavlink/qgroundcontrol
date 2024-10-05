/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <QtCore/QMetaType>

class QGCTile
{
public:
    QGCTile() = default;
    ~QGCTile() = default;

    enum TileState {
        StatePending = 0,
        StateDownloading,
        StateError,
        StateComplete
    };

    int x() const { return m_x; }
    int y() const { return m_y; }
    int z() const { return m_z; }
    quint64 tileSet() const { return m_tileSet;  }
    QString hash() const { return m_hash; }
    QString type() const { return m_type; }

    void setX(int x) { m_x = x; }
    void setY(int y) { m_y = y; }
    void setZ(int z) { m_z = z; }
    void setTileSet(quint64 tileSet) { m_tileSet = tileSet;  }
    void setHash(const QString &hash) { m_hash = hash; }
    void setType(const QString &type) { m_type = type; }

private:
    int m_x = 0;
    int m_y = 0;
    int m_z = 0;
    quint64 m_tileSet = UINT64_MAX;
    QString m_hash;
    QString m_type = QStringLiteral("Invalid");
};
Q_DECLARE_METATYPE(QGCTile)
