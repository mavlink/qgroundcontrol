// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QBUFFER_H
#define QBUFFER_H

#include <QtCore/qiodevice.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class QObject;
class QBufferPrivate;

class Q_CORE_EXPORT QBuffer : public QIODevice
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif

public:
#ifndef QT_NO_QOBJECT
    explicit QBuffer(QObject *parent = nullptr);
    QBuffer(QByteArray *buf, QObject *parent = nullptr);
#else
    QBuffer();
    explicit QBuffer(QByteArray *buf);
#endif
    ~QBuffer();

    QByteArray &buffer();
    const QByteArray &buffer() const;
    void setBuffer(QByteArray *a);

    void setData(const QByteArray &data);
#if QT_CORE_REMOVED_SINCE(6, 5) && QT_POINTER_SIZE != 4
    void setData(const char *data, int len) { setData(data, qsizetype(len)); }
#endif
    void setData(const char *data, qsizetype len);
    const QByteArray &data() const;

    bool open(OpenMode openMode) override;

    void close() override;
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64 off) override;
    bool atEnd() const override;
    bool canReadLine() const override;

protected:
#ifndef QT_NO_QOBJECT
    void connectNotify(const QMetaMethod &) override;
    void disconnectNotify(const QMetaMethod &) override;
#endif
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    Q_DECLARE_PRIVATE(QBuffer)
    Q_DISABLE_COPY(QBuffer)

    Q_PRIVATE_SLOT(d_func(), void _q_emitSignals())
};

QT_END_NAMESPACE

#endif // QBUFFER_H
