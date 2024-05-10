#pragma once

#include <QtCore/QtTypes>
#include <QtCore/QString>
#include <QtCore/QByteArray>

class QGCCacheTile
{
public:
    QGCCacheTile(const QString& hash, const QByteArray& img, const QString& format, const QString& type, qulonglong set = UINT64_MAX)
        : m_set(set)
        , m_hash(hash)
        , m_img(img)
        , m_format(format)
        , m_type(type)
    {}

    QGCCacheTile(const QString& hash, qulonglong set)
        : m_set(set)
        , m_hash(hash)
    {}

    qulonglong set() const { return m_set; }
    QString hash() const { return m_hash; }
    QByteArray img() const { return m_img; }
    QString format() const { return m_format; }
    QString type() const { return m_type; }

private:
    qulonglong m_set;
    QString m_hash;
    QByteArray m_img;
    QString m_format;
    QString m_type;
};

Q_DECLARE_METATYPE(QGCCacheTile)
