#pragma once

#include <QtCore/QtTypes>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMetaType>

class QGCTile
{
public:
    QGCTile() = default;

    enum TileState {
        StatePending = 0,
        StateDownloading,
        StateError,
        StateComplete
    };

    int x() const { return m_x; }
    int y() const { return m_y; }
    int z() const { return m_z; }
    qulonglong tileSet() const { return m_set;  }
    QString hash() const { return m_hash; }
    QString type() const { return m_type; }

    void setX(int x) { m_x = x; }
    void setY(int y) { m_y = y; }
    void setZ(int z) { m_z = z; }
    void setTileSet(qulonglong set) { m_set = set;  }
    void setHash(const QString& hash) { m_hash = hash; }
    void setType(const QString& type) { m_type = type; }

private:
    int m_x = 0;
    int m_y = 0;
    int m_z = 0;
    qulonglong m_set = UINT64_MAX;
    QString m_hash;
    QString m_type = "Invalid";
};

Q_DECLARE_METATYPE(QGCTile)
