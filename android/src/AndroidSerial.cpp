#include "AndroidSerial.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>
#include <QtSerialPort/private/qserialportinfo_p.h>

#include <jni.h>

static Q_LOGGING_CATEGORY(AndroidSerialLog, "qgc.android.serial");
static const char kJniQGCActivityClassName[] {"org/mavlink/qgroundcontrol/QGCActivity"};

static void cleanJavaException()
{
    QJniEnvironment env;
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

void AndroidSerial::onNewData(int port, QByteArray data)
{
    // emit dataReceived(port, data);
}

bool AndroidSerial::isOpen(QString portName)
{
    QJniObject jstr = QJniObject::fromString(portName);

    cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        kJniQGCActivityClassName,
        "isDeviceOpen",
        "(Ljava/lang/String;)Z",
        jstr.object<jstring>()
    );
    cleanJavaException();

    return result;
}

int AndroidSerial::open(QString portName)
{
    QJniObject jstr = QJniObject::fromString(portName);

    cleanJavaException();
    const int deviceId = QJniObject::callStaticMethod<jint>(
        kJniQGCActivityClassName,
        "open",
        "(Ljava/lang/String;J)I",
        jstr.object<jstring>()
    );
    cleanJavaException();

    return deviceId;
}

void AndroidSerial::close(QString portName)
{
    QJniObject jstr = QJniObject::fromString(portName);

    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "close",
        "(Ljava/lang/String;J)V",
        jstr.object<jstring>()
    );
    cleanJavaException();
}

void AndroidSerial::flush(int deviceId, bool input, bool output)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "flush",
        "(IZZ)V",
        deviceId,
        input,
        output
    );
    cleanJavaException();
}

void AndroidSerial::setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "setParameters",
        "(IIIII)V",
        deviceId,
        baudRate,
        dataBits,
        stopBits,
        parity
    );
    cleanJavaException();
}

void AndroidSerial::setDataTerminalReady(int deviceId, bool set)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "setDataTerminalReady",
        "(IZ)V",
        deviceId,
        set
    );
    cleanJavaException();
}

void AndroidSerial::setRequestToSend(int deviceId, bool set)
{
    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "setRequestToSend",
        "(IZ)V",
        deviceId,
        set
    );
    cleanJavaException();
}

void AndroidSerial::write(int deviceId, QByteArrayView data, int length, int timeout, bool async)
{
    QJniEnvironment jniEnv;
    jbyteArray jarray = jniEnv->NewByteArray(static_cast<jsize>(length));
    jniEnv->SetByteArrayRegion(jarray, 0, static_cast<jsize>(length), (jbyte*)data.constData());

    cleanJavaException();
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "write",
        "(I[BI)V",
        deviceId,
        jarray,
        timeout
    );
    cleanJavaException();
}

QByteArray AndroidSerial::read(int deviceId, int length, int timeout)
{
    QJniEnvironment jniEnv;
    jbyteArray jarray = jniEnv->NewByteArray(static_cast<jsize>(length));

    cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        kJniQGCActivityClassName,
        "read",
        "(I[BI)I",
        deviceId,
        jarray,
        timeout
    );
    cleanJavaException();

    QByteArray data = QByteArray::fromRawData(jarray, result);
    data.setRawData(jarray, result);

    return data;
}

QList<QSerialPortInfo> AndroidSerial::availableDevices()
{
    QList<QSerialPortInfo> serialPortInfoList;

    QJniObject result = QJniObject::callStaticObjectMethod(
        kJniQGCActivityClassName,
        "availableDevicesInfo",
        "()[Ljava/lang/String;"
    );

    QJniEnvironment env;
    jobjectArray objArray = result.object<jobjectArray>();
    const int count = env->GetArrayLength(objArray);

    for (int i = 0; i < count; i++)
    {
        jstring string = (jstring) env->GetObjectArrayElement(objArray, i);
        const char *rawString = env->GetStringUTFChars(string, 0);
        QStringList strList = QString::fromUtf8(rawString).split(QStringLiteral(":"));
        env->ReleaseStringUTFChars(string, rawString);
        env->DeleteLocalRef(string);

        QSerialPortInfoPrivate priv;
        priv.portName               = strList.get(0);
        priv.device                 = strList.get(0);
        priv.description            = "";
        priv.manufacturer           = strList.get(1);
        priv.serialNumber           = "";
        priv.vendorIdentifier       = strList.get(3).toInt();
        priv.productIdentifier      = strList.get(2).toInt();
        priv.hasVendorIdentifier    = (priv.vendorIdentifier != 0) ? true: false;
        priv.hasProductIdentifier   = (priv.productIdentifier != 0) ? true: false;

        serialPortInfoList.append(priv);
    }

    return serialPortInfoList;
}
