#include "AndroidSerial.h"
#include "AndroidInterface.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>
#include <QtSerialPort/private/qserialportinfo_p.h>

#include <jni.h>

static Q_LOGGING_CATEGORY(AndroidSerialLog, "qgc.android.serial");

void AndroidSerial::onNewData(int port, QByteArray data)
{
    // emit dataReceived(port, data);
}

QList<QSerialPortInfo> AndroidSerial::availableDevices()
{
    QList<QSerialPortInfo> serialPortInfoList;

    QJniObject result = QJniObject::callStaticObjectMethod(
        AndroidInterface::getQGCActivityClassName(),
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

int AndroidSerial::open(QString portName)
{
    QJniObject jstr = QJniObject::fromString(portName);

    AndroidInterface::cleanJavaException();
    const int deviceId = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getQGCActivityClassName(),
        "open",
        "(Ljava/lang/String;J)I",
        jstr.object<jstring>()
    );
    AndroidInterface::cleanJavaException();

    return deviceId;
}

void AndroidSerial::close(QString portName)
{
    QJniObject jstr = QJniObject::fromString(portName);

    AndroidInterface::cleanJavaException();
    QJniObject::callStaticMethod<void>(
        AndroidInterface::getQGCActivityClassName(),
        "close",
        "(Ljava/lang/String;J)V",
        jstr.object<jstring>()
    );
    AndroidInterface::cleanJavaException();
}

bool AndroidSerial::isOpen(QString portName)
{
    QJniObject jstr = QJniObject::fromString(portName);

    AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getQGCActivityClassName(),
        "isDeviceOpen",
        "(Ljava/lang/String;)Z",
        jstr.object<jstring>()
    );
    AndroidInterface::cleanJavaException();

    return result;
}

QByteArray AndroidSerial::read(int deviceId, int length, int timeout)
{
    QJniEnvironment jniEnv;
    jbyteArray jarray = jniEnv->NewByteArray(static_cast<jsize>(length));

    AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getQGCActivityClassName(),
        "read",
        "(I[BI)I",
        deviceId,
        jarray,
        timeout
    );
    AndroidInterface::cleanJavaException();

    QByteArray data = QByteArray::fromRawData(jarray, result);
    data.setRawData(jarray, result);

    return data;
}

void AndroidSerial::write(int deviceId, QByteArrayView data, int length, int timeout, bool async)
{
    QJniEnvironment jniEnv;
    jbyteArray jarray = jniEnv->NewByteArray(static_cast<jsize>(length));
    jniEnv->SetByteArrayRegion(jarray, 0, static_cast<jsize>(length), (jbyte*)data.constData());

    AndroidInterface::cleanJavaException();
    QJniObject::callStaticMethod<void>(
        AndroidInterface::getQGCActivityClassName(),
        "write",
        "(I[BI)V",
        deviceId,
        jarray,
        timeout
    );
    AndroidInterface::cleanJavaException();
}

void AndroidSerial::setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity)
{
    AndroidInterface::cleanJavaException();
    QJniObject::callStaticMethod<void>(
        AndroidInterface::getQGCActivityClassName(),
        "setParameters",
        "(IIIII)V",
        deviceId,
        baudRate,
        dataBits,
        stopBits,
        parity
    );
    AndroidInterface::cleanJavaException();
}

int AndroidSerial::getCarrierDetect(int deviceId)
{
    AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getQGCActivityClassName(),
        "getCarrierDetect",
        "(I)I",
        deviceId
    );
    AndroidInterface::cleanJavaException();

    return result;
}

int AndroidSerial::getClearToSend(int deviceId)
{
    AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getQGCActivityClassName(),
        "getClearToSend",
        "(I)I",
        deviceId
    );
    AndroidInterface::cleanJavaException();

    return result;
}

int AndroidSerial::getDataSetReady(int deviceId)
{
    AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getQGCActivityClassName(),
        "getDataSetReady",
        "(I)I",
        deviceId
    );
    AndroidInterface::cleanJavaException();

    return result;
}

int AndroidSerial::getDataTerminalReady(int deviceId)
{
    AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getQGCActivityClassName(),
        "getDataTerminalReady",
        "(I)I",
        deviceId
    );
    AndroidInterface::cleanJavaException();

    return result;
}

void AndroidSerial::setDataTerminalReady(int deviceId, bool set)
{
    AndroidInterface::cleanJavaException();
    QJniObject::callStaticMethod<void>(
        AndroidInterface::getQGCActivityClassName(),
        "setDataTerminalReady",
        "(IZ)V",
        deviceId,
        set
    );
    AndroidInterface::cleanJavaException();
}

int AndroidSerial::getRequestToSend(int deviceId)
{
    AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getQGCActivityClassName(),
        "getRequestToSend",
        "(I)I",
        deviceId
    );
    AndroidInterface::cleanJavaException();

    return result;
}

void AndroidSerial::setRequestToSend(int deviceId, bool set)
{
    AndroidInterface::cleanJavaException();
    QJniObject::callStaticMethod<void>(
        AndroidInterface::getQGCActivityClassName(),
        "setRequestToSend",
        "(IZ)V",
        deviceId,
        set
    );
    AndroidInterface::cleanJavaException();
}

void AndroidSerial::flush(int deviceId, bool input, bool output)
{
    AndroidInterface::cleanJavaException();
    QJniObject::callStaticMethod<void>(
        AndroidInterface::getQGCActivityClassName(),
        "flush",
        "(IZZ)V",
        deviceId,
        input,
        output
    );
    AndroidInterface::cleanJavaException();
}

void AndroidSerial::setBreak(int deviceId, bool set)
{
    AndroidInterface::cleanJavaException();
    QJniObject::callStaticMethod<void>(
        AndroidInterface::getQGCActivityClassName(),
        "setBreak",
        "(IZ)V",
        deviceId,
        input,
    );
    AndroidInterface::cleanJavaException();
}