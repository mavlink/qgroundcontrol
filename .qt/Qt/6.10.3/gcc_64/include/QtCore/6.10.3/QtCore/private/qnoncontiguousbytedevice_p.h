// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNONCONTIGUOUSBYTEDEVICE_P_H
#define QNONCONTIGUOUSBYTEDEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qiodevice.h>
#include "private/qringbuffer_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QNonContiguousByteDevice : public QObject
{
    Q_OBJECT
public:
    virtual const char *readPointer(qint64 maximumLength, qint64 &len) = 0;
    virtual bool advanceReadPointer(qint64 amount) = 0;
    virtual bool atEnd() const = 0;
    virtual qint64 pos() const { return -1; }
    virtual bool reset() = 0;
    virtual qint64 size() const = 0;

    virtual ~QNonContiguousByteDevice();

protected:
    QNonContiguousByteDevice();

Q_SIGNALS:
    void readyRead();
    void readProgress(qint64 current, qint64 total);
};

class Q_CORE_EXPORT QNonContiguousByteDeviceFactory
{
public:
    static QNonContiguousByteDevice *create(QIODevice *device);
    static std::shared_ptr<QNonContiguousByteDevice> createShared(QIODevice *device);

    static QNonContiguousByteDevice *create(const QByteArray &byteArray);
    static std::shared_ptr<QNonContiguousByteDevice> createShared(const QByteArray &byteArray);

    static QNonContiguousByteDevice *create(std::shared_ptr<QRingBuffer> ringBuffer);
    static std::shared_ptr<QNonContiguousByteDevice> createShared(std::shared_ptr<QRingBuffer> ringBuffer);

    static QIODevice *wrap(QNonContiguousByteDevice *byteDevice);
};

// the actual implementations
//

class QNonContiguousByteDeviceByteArrayImpl : public QNonContiguousByteDevice
{
    Q_OBJECT
public:
    explicit QNonContiguousByteDeviceByteArrayImpl(QByteArray ba);
    explicit QNonContiguousByteDeviceByteArrayImpl(QBuffer *buffer);
    ~QNonContiguousByteDeviceByteArrayImpl();
    const char *readPointer(qint64 maximumLength, qint64 &len) override;
    bool advanceReadPointer(qint64 amount) override;
    bool atEnd() const override;
    bool reset() override;
    qint64 size() const override;
    qint64 pos() const override;

protected:
    QByteArray byteArray;
    QByteArrayView view;
    qint64 currentPosition = 0;
};

class QNonContiguousByteDeviceRingBufferImpl : public QNonContiguousByteDevice
{
    Q_OBJECT
public:
    explicit QNonContiguousByteDeviceRingBufferImpl(std::shared_ptr<QRingBuffer> rb);
    ~QNonContiguousByteDeviceRingBufferImpl();
    const char *readPointer(qint64 maximumLength, qint64 &len) override;
    bool advanceReadPointer(qint64 amount) override;
    bool atEnd() const override;
    bool reset() override;
    qint64 size() const override;
    qint64 pos() const override;

protected:
    std::shared_ptr<QRingBuffer> ringBuffer;
    qint64 currentPosition = 0;
};

class QNonContiguousByteDeviceIoDeviceImpl : public QNonContiguousByteDevice
{
    Q_OBJECT
public:
    explicit QNonContiguousByteDeviceIoDeviceImpl(QIODevice *d);
    ~QNonContiguousByteDeviceIoDeviceImpl();
    const char *readPointer(qint64 maximumLength, qint64 &len) override;
    bool advanceReadPointer(qint64 amount) override;
    bool atEnd() const override;
    bool reset() override;
    qint64 size() const override;
    qint64 pos() const override;

protected:
    QIODevice *device;
    QByteArray *currentReadBuffer;
    qint64 currentReadBufferSize;
    qint64 currentReadBufferAmount;
    qint64 currentReadBufferPosition;
    qint64 totalAdvancements;
    bool eof;
    qint64 initialPosition;
};

// ... and the reverse thing
class QByteDeviceWrappingIoDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit QByteDeviceWrappingIoDevice(QNonContiguousByteDevice *bd);
    ~QByteDeviceWrappingIoDevice();
    bool isSequential() const override;
    bool atEnd() const override;
    bool reset() override;
    qint64 size() const override;

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

    QNonContiguousByteDevice *byteDevice;
};

QT_END_NAMESPACE

#endif
