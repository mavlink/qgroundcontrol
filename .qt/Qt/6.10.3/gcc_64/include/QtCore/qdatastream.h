// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QDATASTREAM_H
#define QDATASTREAM_H

#include <QtCore/qchar.h>
#include <QtCore/qcontainerfwd.h>
#include <QtCore/qiodevicebase.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qttypetraits.h>

#include <iterator>         // std::distance(), std::next()
#include <memory>

#ifdef Status
#error qdatastream.h must be included before any header file that defines Status
#endif

QT_BEGIN_NAMESPACE

#if QT_CORE_REMOVED_SINCE(6, 3)
class qfloat16;
#endif
class QByteArray;
class QDataStream;
class QIODevice;
class QString;

#if !defined(QT_NO_DATASTREAM)
namespace QtPrivate {
class StreamStateSaver;
template <typename Container>
QDataStream &readArrayBasedContainer(QDataStream &s, Container &c);
template <typename Container>
QDataStream &readListBasedContainer(QDataStream &s, Container &c);
template <typename Container>
QDataStream &readAssociativeContainer(QDataStream &s, Container &c);
template <typename Container>
QDataStream &writeSequentialContainer(QDataStream &s, const Container &c);
template <typename Container>
QDataStream &writeAssociativeContainer(QDataStream &s, const Container &c);
template <typename Container>
QDataStream &writeAssociativeMultiContainer(QDataStream &s, const Container &c);
}
class Q_CORE_EXPORT QDataStream : public QIODeviceBase
{
public:
    enum Version QT7_ONLY(: quint8) {
        Qt_1_0 = 1,
        Qt_2_0 = 2,
        Qt_2_1 = 3,
        Qt_3_0 = 4,
        Qt_3_1 = 5,
        Qt_3_3 = 6,
        Qt_4_0 = 7,
        Qt_4_1 = Qt_4_0,
        Qt_4_2 = 8,
        Qt_4_3 = 9,
        Qt_4_4 = 10,
        Qt_4_5 = 11,
        Qt_4_6 = 12,
        Qt_4_7 = Qt_4_6,
        Qt_4_8 = Qt_4_7,
        Qt_4_9 = Qt_4_8,
        Qt_5_0 = 13,
        Qt_5_1 = 14,
        Qt_5_2 = 15,
        Qt_5_3 = Qt_5_2,
        Qt_5_4 = 16,
        Qt_5_5 = Qt_5_4,
        Qt_5_6 = 17,
        Qt_5_7 = Qt_5_6,
        Qt_5_8 = Qt_5_7,
        Qt_5_9 = Qt_5_8,
        Qt_5_10 = Qt_5_9,
        Qt_5_11 = Qt_5_10,
        Qt_5_12 = 18,
        Qt_5_13 = 19,
        Qt_5_14 = Qt_5_13,
        Qt_5_15 = Qt_5_14,
        Qt_6_0 = 20,
        Qt_6_1 = Qt_6_0,
        Qt_6_2 = Qt_6_0,
        Qt_6_3 = Qt_6_0,
        Qt_6_4 = Qt_6_0,
        Qt_6_5 = Qt_6_0,
        Qt_6_6 = 21,
        Qt_6_7 = 22,
        Qt_6_8 = Qt_6_7,
        Qt_6_9 = Qt_6_7,
        Qt_6_10 = 23,
        Qt_DefaultCompiledVersion = Qt_6_10
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
#error Add the datastream version for this Qt version and update Qt_DefaultCompiledVersion
#endif
    };

    enum ByteOrder {
        BigEndian = QSysInfo::BigEndian,
        LittleEndian = QSysInfo::LittleEndian
    };

    enum Status QT7_ONLY(: quint8) {
        Ok,
        ReadPastEnd,
        ReadCorruptData,
        WriteFailed,
        SizeLimitExceeded,
    };

    enum FloatingPointPrecision QT7_ONLY(: quint8) {
        SinglePrecision,
        DoublePrecision
    };

    QDataStream();
    explicit QDataStream(QIODevice *);
    QDataStream(QByteArray *, OpenMode flags);
    QDataStream(const QByteArray &);
    ~QDataStream();

    QIODevice *device() const;
    void setDevice(QIODevice *);

    bool atEnd() const;

    QT_CORE_INLINE_SINCE(6, 8)
    Status status() const;
    void setStatus(Status status);
    void resetStatus();

    QT_CORE_INLINE_SINCE(6, 8)
    FloatingPointPrecision floatingPointPrecision() const;
    void setFloatingPointPrecision(FloatingPointPrecision precision);

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder);

    int version() const;
    void setVersion(int);

    QDataStream &operator>>(char &i);
    QDataStream &operator>>(qint8 &i);
    QDataStream &operator>>(quint8 &i);
    QDataStream &operator>>(qint16 &i);
    QDataStream &operator>>(quint16 &i);
    QDataStream &operator>>(qint32 &i);
    inline QDataStream &operator>>(quint32 &i);
    QDataStream &operator>>(qint64 &i);
    QDataStream &operator>>(quint64 &i);
    QDataStream &operator>>(std::nullptr_t &ptr) { ptr = nullptr; return *this; }

    QDataStream &operator>>(bool &i);
#if QT_CORE_REMOVED_SINCE(6, 3)
    QDataStream &operator>>(qfloat16 &f);
#endif
    QDataStream &operator>>(float &f);
    QDataStream &operator>>(double &f);
    QDataStream &operator>>(char *&str);
    QDataStream &operator>>(char16_t &c);
    QDataStream &operator>>(char32_t &c);

    QDataStream &operator<<(char i);
    QDataStream &operator<<(qint8 i);
    QDataStream &operator<<(quint8 i);
    QDataStream &operator<<(qint16 i);
    QDataStream &operator<<(quint16 i);
    QDataStream &operator<<(qint32 i);
    inline QDataStream &operator<<(quint32 i);
    QDataStream &operator<<(qint64 i);
    QDataStream &operator<<(quint64 i);
    QDataStream &operator<<(std::nullptr_t) { return *this; }
#if QT_CORE_REMOVED_SINCE(6, 8) || defined(Q_QDOC)
    QDataStream &operator<<(bool i);
#endif
#if !defined(Q_QDOC)
    // Disable implicit conversions to bool (e.g. for pointers)
    template <typename T,
             std::enable_if_t<std::is_same_v<T, bool>, bool> = true>
    QDataStream &operator<<(T i)
    {
        return (*this << qint8(i));
    }
#endif
#if QT_CORE_REMOVED_SINCE(6, 3)
    QDataStream &operator<<(qfloat16 f);
#endif
    QDataStream &operator<<(float f);
    QDataStream &operator<<(double f);
    QDataStream &operator<<(const char *str);
    QDataStream &operator<<(char16_t c);
    QDataStream &operator<<(char32_t c);

    explicit operator bool() const noexcept { return status() == Ok; }

#if QT_DEPRECATED_SINCE(6, 11)
    QT_DEPRECATED_VERSION_X_6_11("Use an overload that takes qint64 length.")
    QDataStream &readBytes(char *&, uint &len);
#endif
#if QT_CORE_REMOVED_SINCE(6, 7)
    QDataStream &writeBytes(const char *, uint len);
    int skipRawData(int len);
    int readRawData(char *, int len);
    int writeRawData(const char *, int len);
#endif
    QDataStream &readBytes(char *&, qint64 &len);
    qint64 readRawData(char *, qint64 len);
    QDataStream &writeBytes(const char *, qint64 len);
    qint64 writeRawData(const char *, qint64 len);
    qint64 skipRawData(qint64 len);

    void startTransaction();
    bool commitTransaction();
    void rollbackTransaction();
    void abortTransaction();

    bool isDeviceTransactionStarted() const;
private:
    Q_DISABLE_COPY(QDataStream)

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    void* const d = nullptr;
#endif

    QIODevice *dev = nullptr;
    bool owndev = false;
    bool noswap = QSysInfo::ByteOrder == QSysInfo::BigEndian;
    quint8 fpPrecision = QDataStream::DoublePrecision;
    quint8 q_status = Ok;
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
    ByteOrder byteorder = BigEndian;
    int ver = Qt_DefaultCompiledVersion;
#else
    Version ver = Qt_DefaultCompiledVersion;
#endif
    quint16 transactionDepth = 0;

#if QT_CORE_REMOVED_SINCE(6, 7)
    int readBlock(char *data, int len);
#endif
    qint64 readBlock(char *data, qint64 len);
    static inline qint64 readQSizeType(QDataStream &s);
    static inline bool writeQSizeType(QDataStream &s, qint64 value);
    static constexpr quint32 NullCode = 0xffffffffu;
    static constexpr quint32 ExtendedSize = 0xfffffffeu;

    friend class QtPrivate::StreamStateSaver;
    Q_CORE_EXPORT friend QDataStream &operator<<(QDataStream &out, const QString &str);
    Q_CORE_EXPORT friend QDataStream &operator>>(QDataStream &in, QString &str);
    Q_CORE_EXPORT friend QDataStream &operator<<(QDataStream &out, const QByteArray &ba);
    Q_CORE_EXPORT friend QDataStream &operator>>(QDataStream &in, QByteArray &ba);
    template <typename Container>
    friend QDataStream &QtPrivate::readArrayBasedContainer(QDataStream &s, Container &c);
    template <typename Container>
    friend QDataStream &QtPrivate::readListBasedContainer(QDataStream &s, Container &c);
    template <typename Container>
    friend QDataStream &QtPrivate::readAssociativeContainer(QDataStream &s, Container &c);
    template <typename Container>
    friend QDataStream &QtPrivate::writeSequentialContainer(QDataStream &s, const Container &c);
    template <typename Container>
    friend QDataStream &QtPrivate::writeAssociativeContainer(QDataStream &s, const Container &c);
    template <typename Container>
    friend QDataStream &QtPrivate::writeAssociativeMultiContainer(QDataStream &s,
                                                                  const Container &c);
};

namespace QtPrivate {

class StreamStateSaver
{
    Q_DISABLE_COPY_MOVE(StreamStateSaver)
public:
    Q_NODISCARD_CTOR
    explicit StreamStateSaver(QDataStream *s) : stream(s), oldStatus(s->status())
    {
        if (!stream->isDeviceTransactionStarted())
            stream->resetStatus();
    }
    inline ~StreamStateSaver()
    {
        if (oldStatus != QDataStream::Ok) {
            stream->resetStatus();
            stream->setStatus(oldStatus);
        }
    }

private:
    QDataStream *stream;
    QDataStream::Status oldStatus;
};

template <typename Container>
QDataStream &readArrayBasedContainer(QDataStream &s, Container &c)
{
    StreamStateSaver stateSaver(&s);

    c.clear();
    qint64 size = QDataStream::readQSizeType(s);
    const auto n = qsizetype(size);
    if (size != n || size < 0) {
        s.setStatus(QDataStream::SizeLimitExceeded);
        return s;
    }
    c.reserve(n);
    for (qsizetype i = 0; i < n; ++i) {
        typename Container::value_type t;
        if (!(s >> t)) {
            c.clear();
            break;
        }
        c.append(t);
    }

    return s;
}

template <typename Container>
QDataStream &readListBasedContainer(QDataStream &s, Container &c)
{
    StreamStateSaver stateSaver(&s);

    c.clear();
    qint64 size = QDataStream::readQSizeType(s);
    const auto n = qsizetype(size);
    if (size != n || size < 0) {
        s.setStatus(QDataStream::SizeLimitExceeded);
        return s;
    }
    for (qsizetype i = 0; i < n; ++i) {
        typename Container::value_type t;
        if (!(s >> t)) {
            c.clear();
            break;
        }
        c << t;
    }

    return s;
}

template <typename Container>
QDataStream &readAssociativeContainer(QDataStream &s, Container &c)
{
    StreamStateSaver stateSaver(&s);

    c.clear();
    qint64 size = QDataStream::readQSizeType(s);
    const auto n = qsizetype(size);
    if (size != n || size < 0) {
        s.setStatus(QDataStream::SizeLimitExceeded);
        return s;
    }
    for (qsizetype i = 0; i < n; ++i) {
        typename Container::key_type k;
        typename Container::mapped_type t;
        if (!(s >> k >> t)) {
            c.clear();
            break;
        }
        c.insert(k, t);
    }

    return s;
}

template <typename Container>
QDataStream &writeSequentialContainer(QDataStream &s, const Container &c)
{
    if (!QDataStream::writeQSizeType(s, c.size()))
        return s;
    for (const typename Container::value_type &t : c)
        s << t;

    return s;
}

template <typename Container>
QDataStream &writeAssociativeContainer(QDataStream &s, const Container &c)
{
    if (!QDataStream::writeQSizeType(s, c.size()))
        return s;
    auto it = c.constBegin();
    auto end = c.constEnd();
    while (it != end) {
        s << it.key() << it.value();
        ++it;
    }

    return s;
}

template <typename Container>
QDataStream &writeAssociativeMultiContainer(QDataStream &s, const Container &c)
{
    if (!QDataStream::writeQSizeType(s, c.size()))
        return s;
    auto it = c.constBegin();
    auto end = c.constEnd();
    while (it != end) {
        const auto rangeStart = it++;
        while (it != end && rangeStart.key() == it.key())
            ++it;
        const qint64 last = std::distance(rangeStart, it) - 1;
        for (qint64 i = last; i >= 0; --i) {
            auto next = std::next(rangeStart, i);
            s << next.key() << next.value();
        }
    }

    return s;
}

} // QtPrivate namespace

template<typename ...T>
using QDataStreamIfHasOStreamOperators =
    std::enable_if_t<std::conjunction_v<QTypeTraits::has_ostream_operator<QDataStream, T>...>, QDataStream &>;
template<typename Container, typename ...T>
using QDataStreamIfHasOStreamOperatorsContainer =
    std::enable_if_t<std::conjunction_v<QTypeTraits::has_ostream_operator_container<QDataStream, Container, T>...>, QDataStream &>;

template<typename ...T>
using QDataStreamIfHasIStreamOperators =
    std::enable_if_t<std::conjunction_v<QTypeTraits::has_istream_operator<QDataStream, T>...>, QDataStream &>;
template<typename Container, typename ...T>
using QDataStreamIfHasIStreamOperatorsContainer =
    std::enable_if_t<std::conjunction_v<QTypeTraits::has_istream_operator_container<QDataStream, Container, T>...>, QDataStream &>;

/*****************************************************************************
  QDataStream inline functions
 *****************************************************************************/

inline QIODevice *QDataStream::device() const
{ return dev; }

#if QT_CORE_INLINE_IMPL_SINCE(6, 8)
QDataStream::Status QDataStream::status() const
{
    return Status(q_status);
}

QDataStream::FloatingPointPrecision QDataStream::floatingPointPrecision() const
{
    return FloatingPointPrecision(fpPrecision);
}
#endif // INLINE_SINCE 6.8

inline QDataStream::ByteOrder QDataStream::byteOrder() const
{
    if constexpr (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        return noswap ? BigEndian : LittleEndian;
    return noswap ? LittleEndian : BigEndian;
}

inline int QDataStream::version() const
{ return ver; }

inline void QDataStream::setVersion(int v)
{ ver = Version(v); }

qint64 QDataStream::readQSizeType(QDataStream &s)
{
    quint32 first;
    s >> first;
    if (first == NullCode)
        return -1;
    if (first < ExtendedSize || s.version() < QDataStream::Qt_6_7)
        return qint64(first);
    qint64 extendedLen;
    s >> extendedLen;
    return extendedLen;
}

bool QDataStream::writeQSizeType(QDataStream &s, qint64 value)
{
    if (value < qint64(ExtendedSize)) {
        s << quint32(value);
    } else if (s.version() >= QDataStream::Qt_6_7) {
        s << ExtendedSize << value;
    } else if (value == qint64(ExtendedSize)) {
        s << ExtendedSize;
    } else {
        s.setStatus(QDataStream::SizeLimitExceeded); // value is too big for old format
        return false;
    }
    return true;
}

inline QDataStream &QDataStream::operator>>(char &i)
{ return *this >> reinterpret_cast<qint8&>(i); }

inline QDataStream &QDataStream::operator>>(quint8 &i)
{ return *this >> reinterpret_cast<qint8&>(i); }

inline QDataStream &QDataStream::operator>>(quint16 &i)
{ return *this >> reinterpret_cast<qint16&>(i); }

inline QDataStream &QDataStream::operator>>(quint32 &i)
{ return *this >> reinterpret_cast<qint32&>(i); }

inline QDataStream &QDataStream::operator>>(quint64 &i)
{ return *this >> reinterpret_cast<qint64&>(i); }

inline QDataStream &QDataStream::operator<<(char i)
{ return *this << qint8(i); }

inline QDataStream &QDataStream::operator<<(quint8 i)
{ return *this << qint8(i); }

inline QDataStream &QDataStream::operator<<(quint16 i)
{ return *this << qint16(i); }

inline QDataStream &QDataStream::operator<<(quint32 i)
{ return *this << qint32(i); }

inline QDataStream &QDataStream::operator<<(quint64 i)
{ return *this << qint64(i); }

template <typename Enum>
inline QDataStream &operator<<(QDataStream &s, QFlags<Enum> e)
{ return s << e.toInt(); }

template <typename Enum>
inline QDataStream &operator>>(QDataStream &s, QFlags<Enum> &e)
{
    typename QFlags<Enum>::Int i;
    s >> i;
    e = QFlags<Enum>::fromInt(i);
    return s;
}

template <typename T>
typename std::enable_if_t<std::is_enum<T>::value, QDataStream &>
operator<<(QDataStream &s, const T &t)
{
    // std::underlying_type_t<T> may be long or ulong, for which QDataStream
    // provides no streaming operators. For those, cast to qint64 or quint64.
    return s << typename QIntegerForSizeof<T>::Unsigned(t);
}

template <typename T>
typename std::enable_if_t<std::is_enum<T>::value, QDataStream &>
operator>>(QDataStream &s, T &t)
{
    typename QIntegerForSizeof<T>::Unsigned i;
    s >> i;
    t = T(i);
    return s;
}

Q_CORE_EXPORT QDataStream &operator<<(QDataStream &out, QChar chr);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &in, QChar &chr);

#ifndef Q_QDOC

template<typename T>
inline QDataStreamIfHasIStreamOperatorsContainer<QList<T>, T> operator>>(QDataStream &s, QList<T> &v)
{
    return QtPrivate::readArrayBasedContainer(s, v);
}

template<typename T>
inline QDataStreamIfHasOStreamOperatorsContainer<QList<T>, T> operator<<(QDataStream &s, const QList<T> &v)
{
    return QtPrivate::writeSequentialContainer(s, v);
}

template <typename T>
inline QDataStreamIfHasIStreamOperatorsContainer<QSet<T>, T> operator>>(QDataStream &s, QSet<T> &set)
{
    return QtPrivate::readListBasedContainer(s, set);
}

template <typename T>
inline QDataStreamIfHasOStreamOperatorsContainer<QSet<T>, T> operator<<(QDataStream &s, const QSet<T> &set)
{
    return QtPrivate::writeSequentialContainer(s, set);
}

template <class Key, class T>
inline QDataStreamIfHasIStreamOperatorsContainer<QHash<Key, T>, Key, T> operator>>(QDataStream &s, QHash<Key, T> &hash)
{
    return QtPrivate::readAssociativeContainer(s, hash);
}

template <class Key, class T>

inline QDataStreamIfHasOStreamOperatorsContainer<QHash<Key, T>, Key, T> operator<<(QDataStream &s, const QHash<Key, T> &hash)
{
    return QtPrivate::writeAssociativeContainer(s, hash);
}

template <class Key, class T>
inline QDataStreamIfHasIStreamOperatorsContainer<QMultiHash<Key, T>, Key, T> operator>>(QDataStream &s, QMultiHash<Key, T> &hash)
{
    return QtPrivate::readAssociativeContainer(s, hash);
}

template <class Key, class T>
inline QDataStreamIfHasOStreamOperatorsContainer<QMultiHash<Key, T>, Key, T> operator<<(QDataStream &s, const QMultiHash<Key, T> &hash)
{
    return QtPrivate::writeAssociativeMultiContainer(s, hash);
}

template <class Key, class T>
inline QDataStreamIfHasIStreamOperatorsContainer<QMap<Key, T>, Key, T> operator>>(QDataStream &s, QMap<Key, T> &map)
{
    return QtPrivate::readAssociativeContainer(s, map);
}

template <class Key, class T>
inline QDataStreamIfHasOStreamOperatorsContainer<QMap<Key, T>, Key, T> operator<<(QDataStream &s, const QMap<Key, T> &map)
{
    return QtPrivate::writeAssociativeContainer(s, map);
}

template <class Key, class T>
inline QDataStreamIfHasIStreamOperatorsContainer<QMultiMap<Key, T>, Key, T> operator>>(QDataStream &s, QMultiMap<Key, T> &map)
{
    return QtPrivate::readAssociativeContainer(s, map);
}

template <class Key, class T>
inline QDataStreamIfHasOStreamOperatorsContainer<QMultiMap<Key, T>, Key, T> operator<<(QDataStream &s, const QMultiMap<Key, T> &map)
{
    return QtPrivate::writeAssociativeMultiContainer(s, map);
}

template <class T1, class T2>
inline QDataStreamIfHasIStreamOperators<T1, T2> operator>>(QDataStream& s, std::pair<T1, T2> &p)
{
    s >> p.first >> p.second;
    return s;
}

template <class T1, class T2>
inline QDataStreamIfHasOStreamOperators<T1, T2> operator<<(QDataStream& s, const std::pair<T1, T2> &p)
{
    s << p.first << p.second;
    return s;
}

#else

template <class T>
QDataStream &operator>>(QDataStream &s, QList<T> &l);

template <class T>
QDataStream &operator<<(QDataStream &s, const QList<T> &l);

template <class T>
QDataStream &operator>>(QDataStream &s, QSet<T> &set);

template <class T>
QDataStream &operator<<(QDataStream &s, const QSet<T> &set);

template <class Key, class T>
QDataStream &operator>>(QDataStream &s, QHash<Key, T> &hash);

template <class Key, class T>
QDataStream &operator<<(QDataStream &s, const QHash<Key, T> &hash);

template <class Key, class T>
QDataStream &operator>>(QDataStream &s, QMultiHash<Key, T> &hash);

template <class Key, class T>
QDataStream &operator<<(QDataStream &s, const QMultiHash<Key, T> &hash);

template <class Key, class T>
QDataStream &operator>>(QDataStream &s, QMap<Key, T> &map);

template <class Key, class T>
QDataStream &operator<<(QDataStream &s, const QMap<Key, T> &map);

template <class Key, class T>
QDataStream &operator>>(QDataStream &s, QMultiMap<Key, T> &map);

template <class Key, class T>
QDataStream &operator<<(QDataStream &s, const QMultiMap<Key, T> &map);

template <class T1, class T2>
QDataStream &operator>>(QDataStream& s, std::pair<T1, T2> &p);

template <class T1, class T2>
QDataStream &operator<<(QDataStream& s, const std::pair<T1, T2> &p);

#endif // Q_QDOC

inline QDataStream &operator>>(QDataStream &s, QKeyCombination &combination)
{
    int combined;
    s >> combined;
    combination = QKeyCombination::fromCombined(combined);
    return s;
}

inline QDataStream &operator<<(QDataStream &s, QKeyCombination combination)
{
    return s << combination.toCombined();
}

#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE

#endif // QDATASTREAM_H
