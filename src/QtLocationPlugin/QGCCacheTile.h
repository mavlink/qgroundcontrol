#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

class QGCCacheTile
{
public:
    QGCCacheTile(const QString &hash, const QByteArray &img, const QString &format, const QString &type, quint64 tileSet = UINT64_MAX)
        : m_tileSet(tileSet)
        , m_hash(hash)
        , m_img(img)
        , m_format(format)
        , m_type(type)
    {}
    QGCCacheTile(const QString &hash, quint64 tileSet)
        : m_tileSet(tileSet)
        , m_hash(hash)
    {}
    ~QGCCacheTile() = default;

    quint64 tileSet() const { return m_tileSet; }
    QString hash() const { return m_hash; }
    QByteArray img() const { return m_img; }
    QString format() const { return m_format; }
    QString type() const { return m_type; }

private:
    quint64 m_tileSet = 0;
    QString m_hash;
    QByteArray m_img;
    QString m_format;
    QString m_type;
};
Q_DECLARE_METATYPE(QGCCacheTile)
