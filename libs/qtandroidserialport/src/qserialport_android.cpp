/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Copyright (C) 2012 Andre Hartmann <aha_1980@gmx.de>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//  Written by: S. Michael Goza 2014
//  Adapted for QGC by: Gus Grubba 2015


#include <errno.h>
#include <stdio.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qmap.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>

#include "qserialport_android_p.h"

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "AndroidSerialPortLog")

QT_BEGIN_NAMESPACE

#define BAD_PORT 0

static const char kJniClassName[] {"org/mavlink/qgroundcontrol/QGCActivity"};

static void jniDeviceHasDisconnected(JNIEnv *envA, jobject thizA, jlong userDataA)
{
    Q_UNUSED(envA);
    Q_UNUSED(thizA);
    if (userDataA != 0)
        (reinterpret_cast<QSerialPortPrivate*>(userDataA))->q_ptr->close();
}

static void jniDeviceNewData(JNIEnv *envA, jobject thizA, jlong userDataA, jbyteArray dataA)
{
    Q_UNUSED(thizA);
    if (userDataA != 0)
    {
        jbyte *bytesL = envA->GetByteArrayElements(dataA, nullptr);
        jsize lenL = envA->GetArrayLength(dataA);
        (reinterpret_cast<QSerialPortPrivate*>(userDataA))->newDataArrived(reinterpret_cast<char*>(bytesL), lenL);
        envA->ReleaseByteArrayElements(dataA, bytesL, JNI_ABORT);
    }
}

static void jniDeviceException(JNIEnv *envA, jobject thizA, jlong userDataA, jstring messageA)
{
    Q_UNUSED(thizA);
    if(userDataA != 0)
    {
        const char *stringL = envA->GetStringUTFChars(messageA, nullptr);
        QString strL = QString::fromUtf8(stringL);
        envA->ReleaseStringUTFChars(messageA, stringL);
        if(envA->ExceptionCheck())
            envA->ExceptionClear();
        (reinterpret_cast<QSerialPortPrivate*>(userDataA))->exceptionArrived(strL);
    }
}

static void jniLogDebug(JNIEnv *envA, jobject thizA, jstring messageA)
{
    Q_UNUSED(thizA);

    const char *stringL = envA->GetStringUTFChars(messageA, nullptr);
    QString logMessage = QString::fromUtf8(stringL);
    envA->ReleaseStringUTFChars(messageA, stringL);
    if (envA->ExceptionCheck())
        envA->ExceptionClear();
    qCDebug(AndroidSerialPortLog) << logMessage;
}

static void jniLogWarning(JNIEnv *envA, jobject thizA, jstring messageA)
{
    Q_UNUSED(thizA);

    const char *stringL = envA->GetStringUTFChars(messageA, nullptr);
    QString logMessage = QString::fromUtf8(stringL);
    envA->ReleaseStringUTFChars(messageA, stringL);
    if (envA->ExceptionCheck())
        envA->ExceptionClear();
    qWarning() << logMessage;
}

void cleanJavaException()
{
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

QSerialPortPrivate::QSerialPortPrivate(QSerialPort *q)
    : QSerialPortPrivateData(q)
    , descriptor(-1)
    , isCustomBaudRateSupported(false)
    , emittedBytesWritten(false)
    , pendingBytesWritten(0)
    , jniDataBits(8)
    , jniStopBits(1)
    , jniParity(0)
    , internalWriteTimeoutMsec(0)
    , isReadStopped(true)
{
}

void QSerialPortPrivate::setNativeMethods(void)
{
    qCDebug(AndroidSerialPortLog) << "Registering Native Functions";

    //  REGISTER THE C++ FUNCTION WITH JNI
    JNINativeMethod javaMethods[] {
        {"nativeDeviceHasDisconnected", "(J)V",                     reinterpret_cast<void *>(jniDeviceHasDisconnected)},
        {"nativeDeviceNewData",         "(J[B)V",                   reinterpret_cast<void *>(jniDeviceNewData)},
        {"nativeDeviceException",       "(JLjava/lang/String;)V",   reinterpret_cast<void *>(jniDeviceException)},
        {"qgcLogDebug",                 "(Ljava/lang/String;)V",    reinterpret_cast<void *>(jniLogDebug)},
        {"qgcLogWarning",               "(Ljava/lang/String;)V",    reinterpret_cast<void *>(jniLogWarning)}
    };

    QAndroidJniEnvironment jniEnv;
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    jclass objectClass = jniEnv->FindClass(kJniClassName);
    if(!objectClass) {
        qWarning() << "Couldn't find class:" << kJniClassName;
        return;
    }

    jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));

    if (val < 0) {
        qWarning() << "Error registering methods: " << val;
    } else {
        qCDebug(AndroidSerialPortLog) << "Native Functions Registered";
    }

    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }
}

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    rwMode = mode;
    qCDebug(AndroidSerialPortLog) << "Opening" << systemLocation.toLatin1().data();

    QAndroidJniObject jnameL = QAndroidJniObject::fromString(systemLocation);
    cleanJavaException();
    deviceId = QAndroidJniObject::callStaticMethod<jint>(
        kJniClassName,
        "open",
        "(Landroid/content/Context;Ljava/lang/String;J)I",
        QtAndroid::androidActivity().object(),
        jnameL.object<jstring>(),
        reinterpret_cast<jlong>(this));
    cleanJavaException();

    isReadStopped = false;

    if (deviceId == BAD_PORT)
    {
        qWarning() << "Error opening" << systemLocation.toLatin1().data();
        q_ptr->setError(QSerialPort::DeviceNotFoundError);
        return false;
    }

    if (rwMode == QIODevice::WriteOnly)
        stopReadThread();

    return true;
}

void QSerialPortPrivate::close()
{
    if (deviceId == BAD_PORT)
        return;

    qCDebug(AndroidSerialPortLog) << "Closing" << systemLocation.toLatin1().data();
    cleanJavaException();
    jboolean resultL = QAndroidJniObject::callStaticMethod<jboolean>(
        kJniClassName,
        "close",
        "(I)Z",
        deviceId);
    cleanJavaException();

    descriptor = -1;
    isCustomBaudRateSupported = false;
    pendingBytesWritten = 0;
    deviceId = BAD_PORT;

    if (!resultL)
        q_ptr->setErrorString(QStringLiteral("Closing device failed"));
}

bool QSerialPortPrivate::setParameters(int baudRateA, int dataBitsA, int stopBitsA, int parityA)
{
    if (deviceId == BAD_PORT)
    {
        q_ptr->setError(QSerialPort::NotOpenError);
        return false;
    }

    cleanJavaException();
    jboolean resultL = QAndroidJniObject::callStaticMethod<jboolean>(
        kJniClassName,
        "setParameters",
        "(IIIII)Z",
        deviceId,
        baudRateA,
        dataBitsA,
        stopBitsA,
        parityA);
    cleanJavaException();

    if(resultL)
    {
        //  SET THE JNI VALUES TO WHAT WAS SENT
        inputBaudRate = outputBaudRate = baudRateA;
        jniDataBits = dataBitsA;
        jniStopBits = stopBitsA;
        jniParity = parityA;
    }

    return resultL;
}



void QSerialPortPrivate::stopReadThread()
{
    if (isReadStopped)
        return;
    cleanJavaException();
    QAndroidJniObject::callStaticMethod<void>(
        kJniClassName,
        "stopIoManager",
        "(I)V",
        deviceId);
    cleanJavaException();
    isReadStopped = true;
}



void QSerialPortPrivate::startReadThread()
{
    if (!isReadStopped)
        return;
    cleanJavaException();
    QAndroidJniObject::callStaticMethod<void>(
        kJniClassName,
        "startIoManager",
        "(I)V",
        deviceId);
    cleanJavaException();
    isReadStopped = false;
}

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    return QSerialPort::NoSignal;
}

bool QSerialPortPrivate::setDataTerminalReady(bool set)
{
    if (deviceId == BAD_PORT)
    {
        q_ptr->setError(QSerialPort::NotOpenError);
        return false;
    }
    cleanJavaException();
    bool res = QAndroidJniObject::callStaticMethod<jboolean>(
        kJniClassName,
        "setDataTerminalReady",
        "(IZ)Z",
        deviceId,
        set);
    cleanJavaException();
    return res;
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    if (deviceId == BAD_PORT)
    {
        q_ptr->setError(QSerialPort::NotOpenError);
        return false;
    }
    cleanJavaException();
    bool res = QAndroidJniObject::callStaticMethod<jboolean>(
        kJniClassName,
        "setRequestToSend",
        "(IZ)Z",
        deviceId,
        set);
    cleanJavaException();
    return res;
}

bool QSerialPortPrivate::flush()
{
    return writeDataOneShot();
}

bool QSerialPortPrivate::clear(QSerialPort::Directions directions)
{
    if (deviceId == BAD_PORT)
    {
        q_ptr->setError(QSerialPort::NotOpenError);
        return false;
    }

    bool inputL = false;
    bool outputL = false;

    if (directions == QSerialPort::AllDirections)
        inputL = outputL = true;
    else
    {
        if (directions & QSerialPort::Input)
            inputL = true;

        if (directions & QSerialPort::Output)
            outputL = true;
    }

    cleanJavaException();
    bool res = QAndroidJniObject::callStaticMethod<jboolean>(
        kJniClassName,
        "purgeBuffers",
        "(IZZ)Z",
        deviceId,
        inputL,
        outputL);

    cleanJavaException();
    return res;
}

bool QSerialPortPrivate::sendBreak(int duration)
{
    Q_UNUSED(duration);
    return true;
}

bool QSerialPortPrivate::setBreakEnabled(bool set)
{
    Q_UNUSED(set);
    return true;
}

void QSerialPortPrivate::startWriting()
{
    writeDataOneShot();
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    int origL = static_cast<int>(readBuffer.size());

    if (origL > 0)
        return true;

    for (int iL=0; iL<msecs; iL++)
    {
        if (origL < readBuffer.size())
            return true;
        else
            QThread::msleep(1);
    }

    return false;
}

bool QSerialPortPrivate::waitForBytesWritten(int msecs)
{
    internalWriteTimeoutMsec = msecs;
    bool retL = writeDataOneShot();
    internalWriteTimeoutMsec = 0;
    return retL;
}

bool QSerialPortPrivate::setBaudRate()
{
    setBaudRate(inputBaudRate, QSerialPort::AllDirections);
    return true;
}

bool QSerialPortPrivate::setBaudRate(qint32 baudRate, QSerialPort::Directions directions)
{
    Q_UNUSED(directions);
    return setParameters(baudRate, jniDataBits, jniStopBits, jniParity);
}

bool QSerialPortPrivate::setDataBits(QSerialPort::DataBits dataBits)
{
    int numBitsL = 8;

    switch (dataBits)
    {
        case QSerialPort::Data5:
            numBitsL = 5;
            break;

        case QSerialPort::Data6:
            numBitsL = 6;
            break;

        case QSerialPort::Data7:
            numBitsL = 7;
            break;

        case QSerialPort::Data8:
        default:
            numBitsL = 8;
            break;
    }
    return setParameters(inputBaudRate, numBitsL, jniStopBits, jniParity);
}

bool QSerialPortPrivate::setParity(QSerialPort::Parity parity)
{
    int parL = 0;
    switch (parity)
    {
        case QSerialPort::SpaceParity:
            parL = 4;
            break;

        case QSerialPort::MarkParity:
            parL = 3;
            break;

        case QSerialPort::EvenParity:
            parL = 2;
            break;

        case QSerialPort::OddParity:
            parL = 1;
            break;

        case QSerialPort::NoParity:
        default:
            parL = 0;
            break;
    }
    return setParameters(inputBaudRate, jniDataBits, jniStopBits, parL);
}

bool QSerialPortPrivate::setStopBits(QSerialPort::StopBits stopBits)
{
    int stopL = 1;
    switch (stopBits)
    {
        case QSerialPort::TwoStop:
            stopL = 2;
            break;

        case QSerialPort::OneAndHalfStop:
            stopL = 3;
            break;

        case QSerialPort::OneStop:
        default:
            stopL = 1;
            break;
    }
    return setParameters(inputBaudRate, jniDataBits, stopL, jniParity);
}

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flowControl)
{
    Q_UNUSED(flowControl);
    return true;
}

bool QSerialPortPrivate::setDataErrorPolicy(QSerialPort::DataErrorPolicy policy)
{
    this->policy = policy;
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void QSerialPortPrivate::newDataArrived(char *bytesA, int lengthA)
{
    Q_Q(QSerialPort);

    int bytesToReadL = lengthA;

    // Always buffered, read data from the port into the read buffer
    if (readBufferMaxSize && (bytesToReadL > (readBufferMaxSize - readBuffer.size()))) {
        bytesToReadL = static_cast<int>(readBufferMaxSize - readBuffer.size());
        if (bytesToReadL <= 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the port.
            stopReadThread();
            return;
        }
    }

    char *ptr = readBuffer.reserve(bytesToReadL);
    memcpy(ptr, bytesA, static_cast<size_t>(bytesToReadL));

    emit q->readyRead();
}



void QSerialPortPrivate::exceptionArrived(QString strA)
{
    q_ptr->setErrorString(strA);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool QSerialPortPrivate::writeDataOneShot()
{
    Q_Q(QSerialPort);

    pendingBytesWritten = -1;

    while (!writeBuffer.isEmpty())
    {
        pendingBytesWritten = writeToPort(writeBuffer.readPointer(), writeBuffer.nextDataBlockSize());

        if (pendingBytesWritten <= 0)
        {
            QSerialPort::SerialPortError errorL = decodeSystemError();
            if (errorL != QSerialPort::ResourceError)
                errorL = QSerialPort::WriteError;
            q->setError(errorL);
            return false;
        }

        writeBuffer.free(pendingBytesWritten);

        emit q->bytesWritten(pendingBytesWritten);
    }

    return (pendingBytesWritten < 0)? false: true;
}

QSerialPort::SerialPortError QSerialPortPrivate::decodeSystemError() const
{
    QSerialPort::SerialPortError error;
    switch (errno) {
        case ENODEV:
            error = QSerialPort::DeviceNotFoundError;
            break;
        case EACCES:
            error = QSerialPort::PermissionError;
            break;
        case EBUSY:
            error = QSerialPort::PermissionError;
            break;
        case EAGAIN:
            error = QSerialPort::ResourceError;
            break;
        case EIO:
            error = QSerialPort::ResourceError;
            break;
        case EBADF:
            error = QSerialPort::ResourceError;
            break;
        default:
            error = QSerialPort::UnknownError;
            break;
    }
    return error;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
qint64 QSerialPortPrivate::writeToPort(const char *data, qint64 maxSize)
{
    if (deviceId == BAD_PORT)
    {
        q_ptr->setError(QSerialPort::NotOpenError);
        return 0;
    }

    QAndroidJniEnvironment jniEnv;
    jbyteArray jarrayL = jniEnv->NewByteArray(static_cast<jsize>(maxSize));
    jniEnv->SetByteArrayRegion(jarrayL, 0, static_cast<jsize>(maxSize), (jbyte*)data);
    if (jniEnv->ExceptionCheck())
        jniEnv->ExceptionClear();
    int resultL = QAndroidJniObject::callStaticMethod<jint>(
        kJniClassName,
        "write",
        "(I[BI)I",
        deviceId,
        jarrayL,
        internalWriteTimeoutMsec);

    if (jniEnv->ExceptionCheck())
    {
        jniEnv->ExceptionClear();
        q_ptr->setErrorString(QStringLiteral("Writing to the device threw an exception"));
        jniEnv->DeleteLocalRef(jarrayL);
        return 0;
    }
    jniEnv->DeleteLocalRef(jarrayL);
    return resultL;
}

typedef QMap<qint32, qint32> BaudRateMap;

// The OS specific defines can be found in termios.h

static const BaudRateMap createStandardBaudRateMap()
{
    BaudRateMap baudRateMap;

#ifdef B50
    baudRateMap.insert(50, B50);
#endif

#ifdef B75
    baudRateMap.insert(75, B75);
#endif

#ifdef B110
    baudRateMap.insert(110, B110);
#endif

#ifdef B134
    baudRateMap.insert(134, B134);
#endif

#ifdef B150
    baudRateMap.insert(150, B150);
#endif

#ifdef B200
    baudRateMap.insert(200, B200);
#endif

#ifdef B300
    baudRateMap.insert(300, B300);
#endif

#ifdef B600
    baudRateMap.insert(600, B600);
#endif

#ifdef B1200
    baudRateMap.insert(1200, B1200);
#endif

#ifdef B1800
    baudRateMap.insert(1800, B1800);
#endif

#ifdef B2400
    baudRateMap.insert(2400, B2400);
#endif

#ifdef B4800
    baudRateMap.insert(4800, B4800);
#endif

#ifdef B7200
    baudRateMap.insert(7200, B7200);
#endif

#ifdef B9600
    baudRateMap.insert(9600, B9600);
#endif

#ifdef B14400
    baudRateMap.insert(14400, B14400);
#endif

#ifdef B19200
    baudRateMap.insert(19200, B19200);
#endif

#ifdef B28800
    baudRateMap.insert(28800, B28800);
#endif

#ifdef B38400
    baudRateMap.insert(38400, B38400);
#endif

#ifdef B57600
    baudRateMap.insert(57600, B57600);
#endif

#ifdef B76800
    baudRateMap.insert(76800, B76800);
#endif

#ifdef B115200
    baudRateMap.insert(115200, B115200);
#endif

#ifdef B230400
    baudRateMap.insert(230400, B230400);
#endif

#ifdef B460800
    baudRateMap.insert(460800, B460800);
#endif

#ifdef B500000
    baudRateMap.insert(500000, B500000);
#endif

#ifdef B576000
    baudRateMap.insert(576000, B576000);
#endif

#ifdef B921600
    baudRateMap.insert(921600, B921600);
#endif

#ifdef B1000000
    baudRateMap.insert(1000000, B1000000);
#endif

#ifdef B1152000
    baudRateMap.insert(1152000, B1152000);
#endif

#ifdef B1500000
    baudRateMap.insert(1500000, B1500000);
#endif

#ifdef B2000000
    baudRateMap.insert(2000000, B2000000);
#endif

#ifdef B2500000
    baudRateMap.insert(2500000, B2500000);
#endif

#ifdef B3000000
    baudRateMap.insert(3000000, B3000000);
#endif

#ifdef B3500000
    baudRateMap.insert(3500000, B3500000);
#endif

#ifdef B4000000
    baudRateMap.insert(4000000, B4000000);
#endif

    return baudRateMap;
}

static const BaudRateMap& standardBaudRateMap()
{
    static const BaudRateMap baudRateMap = createStandardBaudRateMap();
    return baudRateMap;
}

qint32 QSerialPortPrivate::baudRateFromSetting(qint32 setting)
{
    return standardBaudRateMap().key(setting);
}

qint32 QSerialPortPrivate::settingFromBaudRate(qint32 baudRate)
{
    return standardBaudRateMap().value(baudRate);
}

QList<qint32> QSerialPortPrivate::standardBaudRates()
{
    return standardBaudRateMap().keys();
}

QSerialPort::Handle QSerialPort::handle() const
{
    Q_D(const QSerialPort);
    return d->descriptor;
}

qint64 QSerialPortPrivate::bytesToWrite() const
{
    return writeBuffer.size();
}

qint64 QSerialPortPrivate::writeData(const char *data, qint64 maxSize)
{
    return writeToPort(data, maxSize);
}

QT_END_NAMESPACE

