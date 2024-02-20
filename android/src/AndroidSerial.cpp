#include "AndroidSerial.h"
#include "qserialport.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>

#include <jni.h>

#define BAD_PORT 0

static Q_LOGGING_CATEGORY(AndroidSerialLog, "qgc.android.serial");

static void cleanJavaException()
{
    QJniEnvironment env;
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

void AndroidSerial::onDisconnect(int port)
{
    if(port != BAD_PORT)
    {
        close(port);
    }
}

void AndroidSerial::onNewData(int port, QByteArray data)
{
    if(port == BAD_PORT)
    {
        return;
    }

    int bytesToReadL = lengthA;

    if (readBufferMaxSize && (bytesToReadL > (readBufferMaxSize - readBuffer.size())))
    {
        bytesToReadL = static_cast<int>(readBufferMaxSize - readBuffer.size());
        if (bytesToReadL <= 0)
        {
            stopReadThread();
            return;
        }
    }

    char *ptr = readBuffer.reserve(bytesToReadL);
    memcpy(ptr, bytesA, static_cast<size_t>(bytesToReadL));

    emit q->readyRead();
}

void AndroidSerial::onException(int port, QStringView msg)
{
    if(deviceId != BAD_PORT)
    {
        qCWarning(AndroidSerialLog) << deviceId << ":" << msg;
    }
}

bool AndroidSerial::open(QString portName)
{
    cleanJavaException();
    deviceId = QJniObject::callStaticMethod<jint>(
        kJniQGCActivityClassName,
        "open",
        "(Ljava/lang/String;J)I",
        jnameL.object<jstring>(),
        reinterpret_cast<jlong>(this)
    );
    cleanJavaException();

    return true;
}

bool AndroidSerial::flush(int deviceId)
{
    if (deviceId == BAD_PORT)
    {
        return false;
    }

    cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        kJniQGCActivityClassName,
        "close",
        "(I)Z",
        deviceId
    );
    cleanJavaException();

    return result;
}

bool AndroidSerial::setParameters(int deviceId, int baudRateA, int dataBitsA, int stopBitsA, int parityA)
{
    if (deviceId == BAD_PORT)
    {
        return false;
    }

    cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        kJniQGCActivityClassName,
        "setParameters",
        "(IIIII)Z",
        deviceId,
        baudRateA,
        dataBitsA,
        stopBitsA,
        parityA
    );
    cleanJavaException();

    return result;
}

bool AndroidSerial::setDataTerminalReady(int deviceId, bool set)
{
    if (deviceId == BAD_PORT)
    {
        return false;
    }

    cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        kJniQGCActivityClassName,
        "setDataTerminalReady",
        "(IZ)Z",
        deviceId,
        set
    );
    cleanJavaException();

    return result;
}

bool AndroidSerial::setRequestToSend(int deviceId, bool set)
{
    if (deviceId == BAD_PORT)
    {
        return false;
    }

    cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        kJniQGCActivityClassName,
        "setRequestToSend",
        "(IZ)Z",
        deviceId,
        set
    );
    cleanJavaException();

    return res;
}

bool AndroidSerial::clear(int deviceId, bool input, bool output)
{
    if (deviceId == BAD_PORT)
    {
        return false;
    }

    cleanJavaException();
    const bool res = QJniObject::callStaticMethod<jboolean>(
        kJniQGCActivityClassName,
        "purgeBuffers",
        "(IZZ)Z",
        deviceId,
        input,
        output
    );
    cleanJavaException();

    return res;
}

size_t AndroidSerial::write(int deviceId, const char *data, size_t length)
{
    if (deviceId == BAD_PORT)
    {
        return 0;
    }

    QJniEnvironment jniEnv;
    jbyteArray jarrayL = jniEnv->NewByteArray(static_cast<jsize>(length));
    jniEnv->SetByteArrayRegion(jarrayL, 0, static_cast<jsize>(length), (jbyte*)data);

    cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        kJniQGCActivityClassName,
        "write",
        "(I[BI)I",
        deviceId,
        jarrayL,
        internalWriteTimeoutMsec
    );
    cleanJavaException();

    return result;
}

size_t AndroidSerial::read(int deviceId, char *data, size_t length)
{

}

bool AndroidSerial::isOpen(int deviceId)
{
    cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        V_jniClassName,
        "isDeviceNameOpen",
        "(Ljava/lang/String;)Z",
        jstrL.object<jstring>()
    );
    cleanJavaException();

    return result;
}

QList<QSerialPortInfo> AndroidSerial::availableDevices()
{
    QList<QSerialPortInfo> serialPortInfoList;

    QJniObject resultL = QJniObject::callStaticObjectMethod(
        V_jniClassName,
        "availableDevicesInfo",
        "()[Ljava/lang/String;"
    );

    if (!resultL.isValid())
    {
        return serialPortInfoList;
    }

    QJniEnvironment envL;
    jobjectArray objArrayL = resultL.object<jobjectArray>();
    int countL = envL->GetArrayLength(objArrayL);

    for (int iL = 0; iL < countL; iL++)
    {
        QSerialPortInfoPrivate priv;
        jstring stringL = (jstring)(envL->GetObjectArrayElement(objArrayL, iL));
        const char *rawStringL = envL->GetStringUTFChars(stringL, 0);
        QStringList strListL = QString::fromUtf8(rawStringL).split(QStringLiteral(":"));
        envL->ReleaseStringUTFChars(stringL, rawStringL);
        envL->DeleteLocalRef(stringL);

        priv.portName               = strListL[0];
        priv.device                 = strListL[0];
        priv.manufacturer           = strListL[1];
        priv.productIdentifier      = strListL[2].toInt();
        priv.hasProductIdentifier   = (priv.productIdentifier != 0) ? true: false;
        priv.vendorIdentifier       = strListL[3].toInt();
        priv.hasVendorIdentifier    = (priv.vendorIdentifier  != 0) ? true: false;

        serialPortInfoList.append(priv);
    }

    return serialPortInfoList;
}

// Port Access Functions

void AndroidSerial::startPort(int port)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "startPort",
        "(I)V",
        port
    );
    cleanJavaException();
}

void AndroidSerial::stopPort(int port)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "stopPort",
        "(I)V",
        port
    );
    cleanJavaException();
}

bool AndroidSerial::isPortRunning(int port)
{
    cleanJavaException();
    const bool result = QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "isPortRunning",
        "(I)V",
        port
    );
    cleanJavaException();

    return result;
}

int AndroidSerial::getPortReadBufferSize(int port)
{
    cleanJavaException();
    const int size = QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "getPortReadBufferSize",
        "(I)V",
        port
    );
    cleanJavaException();

    return size;
}

void AndroidSerial::setPortReadBufferSize(int port, int size)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "setPortReadBufferSize",
        "(I)V",
        port,
        size
    );
    cleanJavaException();
}

int AndroidSerial::getPortWriteBufferSize(int port)
{
    cleanJavaException();
    const int size = QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "getPortWriteBufferSize",
        "(I)V",
        port
    );
    cleanJavaException();

    return size;
}

void AndroidSerial::setPortWriteBufferSize(int port, int size)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "setPortWriteBufferSize",
        "(I)V",
        port,
        size
    );
    cleanJavaException();
}

int AndroidSerial::getPortReadTimeout(int port)
{
    cleanJavaException();
    const int timeout = QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "getPortReadTimeout",
        "(I)V",
        port
    );
    cleanJavaException();

    return timeout;
}

void AndroidSerial::setPortReadTimeout(int port, int timeout)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "setPortReadTimeout",
        "(I)V",
        port,
        timeout
    );
    cleanJavaException();
}

int AndroidSerial::getPortWriteTimeout(int port)
{
    cleanJavaException();
    const int timeout = QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "getPortWriteTimeout",
        "(I)V",
        port
    );
    cleanJavaException();

    return timeout;
}

void AndroidSerial::setPortWriteTimeout(int port, int timeout)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "setPortWriteTimeout",
        "(I)V",
        port,
        timeout
    );
    cleanJavaException();
}
