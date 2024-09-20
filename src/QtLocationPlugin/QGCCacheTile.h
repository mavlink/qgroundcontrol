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
    const QString &hash() const { return m_hash; }
    const QByteArray &img() const { return m_img; }
    const QString &format() const { return m_format; }
    const QString &type() const { return m_type; }

private:
    const quint64 m_tileSet = 0;
    const QString m_hash;
    const QByteArray m_img;
    const QString m_format;
    const QString m_type;
};
Q_DECLARE_METATYPE(QGCCacheTile)
