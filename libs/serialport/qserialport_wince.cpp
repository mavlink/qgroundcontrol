/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Copyright (C) 2012 Andre Hartmann <aha_1980@gmx.de>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qserialport_win_p.h"

#include <QtCore/qelapsedtimer.h>

#include <QtCore/qthread.h>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

class QSerialPortPrivate;

class CommEventNotifier : public QThread
{
    Q_OBJECT
signals:
    void eventMask(quint32 mask);

public:
    CommEventNotifier(DWORD mask, QSerialPortPrivate *d, QObject *parent)
        : QThread(parent), dptr(d), running(true) {
        connect(this, SIGNAL(eventMask(quint32)), this, SLOT(processNotification(quint32)));
        ::SetCommMask(dptr->descriptor, mask);
    }

    virtual ~CommEventNotifier() {
        running = false;
        ::SetCommMask(dptr->descriptor, 0);
        wait();
    }

protected:
    void run() Q_DECL_OVERRIDE {
        DWORD mask = 0;
        while (running) {
            if (::WaitCommEvent(dptr->descriptor, &mask, FALSE)) {
                // Wait until complete the operation changes the port settings,
                // see updateDcb().
                dptr->settingsChangeMutex.lock();
                dptr->settingsChangeMutex.unlock();
                emit eventMask(quint32(mask));
            }
        }
    }

private slots:
    void processNotification(quint32 eventMask) {

        bool error = false;

        // Check for unexpected event. This event triggered when pulled previously
        // opened device from the system, when opened as for not to read and not to
        // write options and so forth.
        if ((eventMask == 0)
                || ((eventMask & (EV_ERR | EV_RXCHAR | EV_TXEMPTY)) == 0)) {
            error = true;
        }

        if (error || (EV_ERR & eventMask))
            dptr->processIoErrors(error);
        if (EV_RXCHAR & eventMask)
            dptr->notifyRead();
        if (EV_TXEMPTY & eventMask)
            dptr->notifyWrite(QSerialPortPrivateData::WriteChunkSize);
    }

private:
    QSerialPortPrivate *dptr;
    mutable bool running;
};

class WaitCommEventBreaker : public QThread
{
    Q_OBJECT
public:
    WaitCommEventBreaker(HANDLE descriptor, int timeout, QObject *parent = 0)
        : QThread(parent), descriptor(descriptor), timeout(timeout), worked(false) {
        start();
    }

    virtual ~WaitCommEventBreaker() {
        stop();
        wait();
    }

    void stop() {
        exit(0);
    }

    bool isWorked() const {
        return worked;
    }

protected:
    void run() {
        QTimer timer;
        QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(processTimeout()), Qt::DirectConnection);
        timer.start(timeout);
        exec();
        worked = true;
    }

private slots:
    void processTimeout() {
        ::SetCommMask(descriptor, 0);
        stop();
    }

private:
    HANDLE descriptor;
    int timeout;
    mutable bool worked;
};

QSerialPortPrivate::QSerialPortPrivate(QSerialPort *q)
    : QSerialPortPrivateData(q)
    , descriptor(INVALID_HANDLE_VALUE)
    , parityErrorOccurred(false)
    , eventNotifier(0)
{
}

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    DWORD desiredAccess = 0;
    DWORD eventMask = EV_ERR;

    if (mode & QIODevice::ReadOnly) {
        desiredAccess |= GENERIC_READ;
        eventMask |= EV_RXCHAR;
    }
    if (mode & QIODevice::WriteOnly) {
        desiredAccess |= GENERIC_WRITE;
        eventMask |= EV_TXEMPTY;
    }

    descriptor = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation.utf16()),
                              desiredAccess, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (descriptor == INVALID_HANDLE_VALUE) {
        q_ptr->setError(decodeSystemError());
        return false;
    }

    if (!::GetCommState(descriptor, &restoredDcb)) {
        q_ptr->setError(decodeSystemError());
        return false;
    }

    currentDcb = restoredDcb;
    currentDcb.fBinary = true;
    currentDcb.fInX = false;
    currentDcb.fOutX = false;
    currentDcb.fAbortOnError = false;
    currentDcb.fNull = false;
    currentDcb.fErrorChar = false;

    if (!updateDcb())
        return false;

    if (!::GetCommTimeouts(descriptor, &restoredCommTimeouts)) {
        q_ptr->setError(decodeSystemError());
        return false;
    }

    ::memset(&currentCommTimeouts, 0, sizeof(currentCommTimeouts));
    currentCommTimeouts.ReadIntervalTimeout = MAXDWORD;

    if (!updateCommTimeouts())
        return false;

    eventNotifier = new CommEventNotifier(eventMask, this, q_ptr);
    eventNotifier->start();

    detectDefaultSettings();
    return true;
}

void QSerialPortPrivate::close()
{
    if (eventNotifier) {
        eventNotifier->deleteLater();
        eventNotifier = 0;
    }

    if (settingsRestoredOnClose) {
        ::SetCommState(descriptor, &restoredDcb);
        ::SetCommTimeouts(descriptor, &restoredCommTimeouts);
    }

    ::CloseHandle(descriptor);
    descriptor = INVALID_HANDLE_VALUE;
}

bool QSerialPortPrivate::flush()
{
    return notifyWrite() && ::FlushFileBuffers(descriptor);
}

bool QSerialPortPrivate::clear(QSerialPort::Directions dir)
{
    DWORD flags = 0;
    if (dir & QSerialPort::Input)
        flags |= PURGE_RXABORT | PURGE_RXCLEAR;
    if (dir & QSerialPort::Output)
        flags |= PURGE_TXABORT | PURGE_TXCLEAR;
    return ::PurgeComm(descriptor, flags);
}

qint64 QSerialPortPrivate::bytesAvailable() const
{
    return readBuffer.size();
}

qint64 QSerialPortPrivate::readFromBuffer(char *data, qint64 maxSize)
{
    if (readBuffer.isEmpty())
        return 0;

    if (maxSize == 1) {
        *data = readBuffer.getChar();
        return 1;
    }

    const qint64 bytesToRead = qMin(qint64(readBuffer.size()), maxSize);
    qint64 readSoFar = 0;
    while (readSoFar < bytesToRead) {
        const char *ptr = readBuffer.readPointer();
        const int bytesToReadFromThisBlock = qMin(int(bytesToRead - readSoFar),
                                                  readBuffer.nextDataBlockSize());
        ::memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
        readSoFar += bytesToReadFromThisBlock;
        readBuffer.free(bytesToReadFromThisBlock);
    }

    return readSoFar;
}

qint64 QSerialPortPrivate::writeToBuffer(const char *data, qint64 maxSize)
{
    char *ptr = writeBuffer.reserve(maxSize);
    if (maxSize == 1)
        *ptr = *data;
    else
        ::memcpy(ptr, data, maxSize);

    // trigger write sequence
    notifyWrite(QSerialPortPrivateData::WriteChunkSize);

    return maxSize;
}

bool QSerialPortPrivate::waitForReadyRead(int msec)
{
    if (!readBuffer.isEmpty())
        return true;

    QElapsedTimer stopWatch;

    stopWatch.start();

    forever {
        bool readyToRead = false;
        bool readyToWrite = false;
        bool timedOut = false;
        if (!waitForReadOrWrite(&readyToRead, &readyToWrite,
                                true, !writeBuffer.isEmpty(),
                                timeoutValue(msec, stopWatch.elapsed()),
                                &timedOut)) {
            return false;
        }
        if (readyToRead) {
            if (notifyRead())
                return true;
        }
        if (readyToWrite)
            notifyWrite(WriteChunkSize);
    }
    return false;
}

bool QSerialPortPrivate::waitForBytesWritten(int msec)
{
    if (writeBuffer.isEmpty())
        return false;

    QElapsedTimer stopWatch;

    stopWatch.start();

    forever {
        bool readyToRead = false;
        bool readyToWrite = false;
        bool timedOut = false;
        if (!waitForReadOrWrite(&readyToRead, &readyToWrite,
                                true, !writeBuffer.isEmpty(),
                                timeoutValue(msec, stopWatch.elapsed()),
                                &timedOut)) {
            return false;
        }
        if (readyToRead) {
            if (!notifyRead())
                return false;
        }
        if (readyToWrite) {
            if (notifyWrite(WriteChunkSize))
                return true;
        }
    }
    return false;
}

bool QSerialPortPrivate::notifyRead()
{
    DWORD bytesToRead = (policy == QSerialPort::IgnorePolicy) ? ReadChunkSize : 1;

    if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - readBuffer.size())) {
        bytesToRead = readBufferMaxSize - readBuffer.size();
        if (bytesToRead == 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the port.
            return false;
        }
    }

    char *ptr = readBuffer.reserve(bytesToRead);

    DWORD readBytes = 0;
    BOOL sucessResult = ::ReadFile(descriptor, ptr, bytesToRead, &readBytes, NULL);

    if (!sucessResult) {
        readBuffer.truncate(bytesToRead);
        q_ptr->setError(QSerialPort::ReadError);
        return false;
    }

    readBuffer.truncate(readBytes);

    // Process emulate policy.
    if ((policy != QSerialPort::IgnorePolicy) && parityErrorOccurred) {

        parityErrorOccurred = false;

        switch (policy) {
        case QSerialPort::SkipPolicy:
            readBuffer.getChar();
            return true;
        case QSerialPort::PassZeroPolicy:
            readBuffer.getChar();
            readBuffer.putChar('\0');
            break;
        case QSerialPort::StopReceivingPolicy:
            // FIXME: Maybe need disable read notifier?
            break;
        default:
            break;
        }
    }

    if (readBytes > 0)
        emit q_ptr->readyRead();

    return true;
}

bool QSerialPortPrivate::notifyWrite(int maxSize)
{
    int nextSize = qMin(writeBuffer.nextDataBlockSize(), maxSize);

    const char *ptr = writeBuffer.readPointer();

    DWORD bytesWritten = 0;
    if (!::WriteFile(descriptor, ptr, nextSize, &bytesWritten, NULL)) {
        q_ptr->setError(QSerialPort::WriteError);
        return false;
    }

    writeBuffer.free(bytesWritten);

    if (bytesWritten > 0)
        emit q_ptr->bytesWritten(bytesWritten);

    return true;
}

bool QSerialPortPrivate::waitForReadOrWrite(bool *selectForRead, bool *selectForWrite,
                                           bool checkRead, bool checkWrite,
                                           int msecs, bool *timedOut)
{
    DWORD eventMask = 0;
    // FIXME: Here the situation is not properly handled with zero timeout:
    // breaker can work out before you call a method WaitCommEvent()
    // and so it will loop forever!
    WaitCommEventBreaker breaker(descriptor, qMax(msecs, 0));
    ::WaitCommEvent(descriptor, &eventMask, NULL);
    breaker.stop();

    if (breaker.isWorked())
        *timedOut = true;

    if (!breaker.isWorked()) {
        if (checkRead) {
            Q_ASSERT(selectForRead);
            *selectForRead = eventMask & EV_RXCHAR;
        }
        if (checkWrite) {
            Q_ASSERT(selectForWrite);
            *selectForWrite = eventMask & EV_TXEMPTY;
        }

        return true;
    }

    return false;
}

bool QSerialPortPrivate::updateDcb()
{
    QMutexLocker locker(&settingsChangeMutex);

    DWORD eventMask = 0;
    // Save the event mask
    if (!::GetCommMask(descriptor, &eventMask))
        return false;

    // Break event notifier from WaitCommEvent
    ::SetCommMask(descriptor, 0);
    // Change parameters
    bool ret = ::SetCommState(descriptor, &currentDcb);
    if (!ret)
        q_ptr->setError(decodeSystemError());
    // Restore the event mask
    ::SetCommMask(descriptor, eventMask);

    return ret;
}

bool QSerialPortPrivate::updateCommTimeouts()
{
    if (!::SetCommTimeouts(descriptor, &currentCommTimeouts)) {
        q_ptr->setError(decodeSystemError());
        return false;
    }
    return true;
}

static const QLatin1String defaultPathPostfix(":");

QString QSerialPortPrivate::portNameToSystemLocation(const QString &port)
{
    QString ret = port;
    if (!ret.contains(defaultPathPostfix))
        ret.append(defaultPathPostfix);
    return ret;
}

QString QSerialPortPrivate::portNameFromSystemLocation(const QString &location)
{
    QString ret = location;
    if (ret.contains(defaultPathPostfix))
        ret.remove(defaultPathPostfix);
    return ret;
}

#include "qserialport_wince.moc"

QT_END_NAMESPACE
