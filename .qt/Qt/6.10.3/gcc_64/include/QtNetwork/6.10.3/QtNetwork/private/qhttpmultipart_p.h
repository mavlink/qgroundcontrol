// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPMULTIPART_P_H
#define QHTTPMULTIPART_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QtNetwork/qhttpmultipart.h>

#include "QtCore/qshareddata.h"
#include "qnetworkrequest_p.h" // for deriving QHttpPartPrivate from QNetworkHeadersPrivate
#include "qhttpheadershelper_p.h"

#include "private/qobject_p.h"
#include <QtCore/qiodevice.h>

#ifndef Q_OS_WASM
QT_REQUIRE_CONFIG(http);
#endif

QT_BEGIN_NAMESPACE


class QHttpPartPrivate: public QSharedData, public QNetworkHeadersPrivate
{
public:
    inline QHttpPartPrivate() : bodyDevice(nullptr), headerCreated(false), readPointer(0)
    {
    }
    ~QHttpPartPrivate()
    {
    }


    QHttpPartPrivate(const QHttpPartPrivate &other)
        : QSharedData(other), QNetworkHeadersPrivate(other), body(other.body),
        header(other.header), headerCreated(other.headerCreated), readPointer(other.readPointer)
    {
        bodyDevice = other.bodyDevice;
    }

    inline bool operator==(const QHttpPartPrivate &other) const
    {
        return QHttpHeadersHelper::compareStrict(httpHeaders, other.httpHeaders)
               && body == other.body
               && bodyDevice == other.bodyDevice
               && readPointer == other.readPointer;
    }

    void setBodyDevice(QIODevice *device) {
        bodyDevice = device;
        readPointer = 0;
    }
    void setBody(const QByteArray &newBody) {
        body = newBody;
        readPointer = 0;
    }

    // QIODevice-style methods called by QHttpMultiPartIODevice (but this class is
    // not a QIODevice):
    qint64 bytesAvailable() const;
    qint64 readData(char *data, qint64 maxSize);
    qint64 size() const;
    bool reset();

    QByteArray body;
    QIODevice *bodyDevice;

private:
    void checkHeaderCreated() const;

    mutable QByteArray header;
    mutable bool headerCreated;
    qint64 readPointer;
};



class QHttpMultiPartPrivate;

class Q_AUTOTEST_EXPORT QHttpMultiPartIODevice : public QIODevice
{
public:
    QHttpMultiPartIODevice(QHttpMultiPartPrivate *parentMultiPart) :
            QIODevice(), multiPart(parentMultiPart), readPointer(0), deviceSize(-1) {
    }

    ~QHttpMultiPartIODevice() override;

    virtual bool atEnd() const override {
        return readPointer == size();
    }

    virtual qint64 bytesAvailable() const override {
        return size() - readPointer;
    }

    virtual void close() override {
        readPointer = 0;
        partOffsets.clear();
        deviceSize = -1;
        QIODevice::close();
    }

    virtual qint64 bytesToWrite() const override {
        return 0;
    }

    virtual qint64 size() const override;
    virtual bool isSequential() const override;
    virtual bool reset() override;
    virtual qint64 readData(char *data, qint64 maxSize) override;
    virtual qint64 writeData(const char *data, qint64 maxSize) override;

    QHttpMultiPartPrivate *multiPart;
    qint64 readPointer;
    mutable QList<qint64> partOffsets;
    mutable qint64 deviceSize;
};



class Q_AUTOTEST_EXPORT QHttpMultiPartPrivate: public QObjectPrivate
{
public:

    QHttpMultiPartPrivate();
    ~QHttpMultiPartPrivate() override;

    static QHttpMultiPartPrivate *get(QHttpMultiPart *message) { return message->d_func(); }
    static const QHttpMultiPartPrivate *get(const QHttpMultiPart *message)
    {
        return message->d_func();
    }

    QList<QHttpPart> parts;
    QByteArray boundary;
    QHttpMultiPart::ContentType contentType;
    QHttpMultiPartIODevice *device;

};

QT_END_NAMESPACE


#endif // QHTTPMULTIPART_P_H
