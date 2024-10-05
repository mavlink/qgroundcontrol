#include "AndroidSerial.h"
#include "AndroidInterface.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>
#include <QtCore/QCoreApplication>

#include <qserialport_p.h>
#include <qserialportinfo_p.h>

#include <jni.h>

Q_LOGGING_CATEGORY(AndroidSerialLog, "qgc.android.serial");

namespace AndroidSerial
{

void setNativeMethods()
{
    qCDebug(AndroidSerialLog) << "Registering Native Functions";

    JNINativeMethod javaMethods[] {
        {"nativeDeviceHasDisconnected", "(J)V",                     reinterpret_cast<void*>(jniDeviceHasDisconnected)},
        {"nativeDeviceNewData",         "(J[B)V",                   reinterpret_cast<void*>(jniDeviceNewData)},
        {"nativeDeviceException",       "(JLjava/lang/String;)V",   reinterpret_cast<void*>(jniDeviceException)},
    };

    (void) AndroidInterface::cleanJavaException();

    jclass objectClass = AndroidInterface::getActivityClass();
    if (!objectClass) {
        qCWarning(AndroidSerialLog) << "Couldn't find class:" << objectClass;
        return;
    }

    QJniEnvironment jniEnv;
    jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));

    if (val < 0) {
        qCWarning(AndroidSerialLog) << "Error registering methods: " << val;
    } else {
        qCDebug(AndroidSerialLog) << "Native Functions Registered";
    }

    (void) jniEnv.checkAndClearExceptions();
}

void jniDeviceHasDisconnected(JNIEnv *env, jobject obj, jlong classPtr)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);

    if (classPtr != 0) {
        QSerialPortPrivate* const serialPortPrivate = reinterpret_cast<QSerialPortPrivate*>(classPtr);
        qCDebug(AndroidSerialLog) << "Device disconnected" << serialPortPrivate->systemLocation.toLatin1().constData();
        QSerialPort* const serialPort = static_cast<QSerialPort*>(serialPortPrivate->q_ptr);
        serialPort->close();
    }
}

void jniDeviceNewData(JNIEnv *env, jobject obj, jlong classPtr, jbyteArray data)
{
    Q_UNUSED(obj);

    if (classPtr != 0) {
        jbyte* const bytes = env->GetByteArrayElements(data, nullptr);
        const jsize len = env->GetArrayLength(data);
        // QByteArray data = QByteArray::fromRawData(reinterpret_cast<char*>(bytes), len);
        QSerialPortPrivate* const serialPort = reinterpret_cast<QSerialPortPrivate*>(classPtr);
        serialPort->newDataArrived(reinterpret_cast<char*>(bytes), len);
        env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
        (void) QJniEnvironment::checkAndClearExceptions(env);
    }
}

void jniDeviceException(JNIEnv *env, jobject obj, jlong classPtr, jstring message)
{
    Q_UNUSED(obj);

    if (classPtr != 0) {
        const char* const string = env->GetStringUTFChars(message, nullptr);
        const QString str = QString::fromUtf8(string);
        QSerialPortPrivate* const serialPort = reinterpret_cast<QSerialPortPrivate*>(classPtr);
        serialPort->exceptionArrived(str);
        env->ReleaseStringUTFChars(message, string);
        (void) QJniEnvironment::checkAndClearExceptions(env);
    }
}

QList<QSerialPortInfo> availableDevices()
{
    jclass javaClass = AndroidInterface::getActivityClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "Class Not Found";
        return QList<QSerialPortInfo>();
    }

    QJniEnvironment env;
    const jmethodID methodId = env.findStaticMethod(javaClass, "availableDevicesInfo", "()[Ljava/lang/String;");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found";
        return QList<QSerialPortInfo>();
    }

    const QJniObject result = QJniObject::callStaticObjectMethod(javaClass, methodId);
    if (!result.isValid()) {
        qCDebug(AndroidSerialLog) << "Method Call Failed";
        return QList<QSerialPortInfo>();
    }

    QList<QSerialPortInfo> serialPortInfoList;

    const jobjectArray objArray = result.object<jobjectArray>();
    const jsize count = env->GetArrayLength(objArray);

    for (jsize i = 0; i < count; i++) {
        jobject obj = env->GetObjectArrayElement(objArray, i);
        jstring string = static_cast<jstring>(obj);
        const char * const rawString = env->GetStringUTFChars(string, 0);
        qCDebug(AndroidSerialLog) << "Adding device" << rawString;

        const QStringList strList = QString::fromUtf8(rawString).split(QStringLiteral(":"));
        env->ReleaseStringUTFChars(string, rawString);
        env->DeleteLocalRef(string);

        if (strList.size() < 6) {
            qCWarning(AndroidSerialLog) << "Invalid device info";
            continue;
        }

        QSerialPortInfoPrivate priv;

        priv.portName = QSerialPortInfoPrivate::portNameFromSystemLocation(strList.at(0));
        priv.device = strList.at(0);
        priv.description = strList.at(1);
        priv.manufacturer = strList.at(2);
        priv.serialNumber = strList.at(3);
        priv.productIdentifier = strList.at(4).toInt();
        priv.hasProductIdentifier = (priv.productIdentifier != BAD_PORT);
        priv.vendorIdentifier = strList.at(5).toInt();
        priv.hasVendorIdentifier = (priv.vendorIdentifier != BAD_PORT);

        (void) serialPortInfoList.append(priv);
    }
    (void) env.checkAndClearExceptions();

    return serialPortInfoList;
}

int getDeviceId(const QString &portName)
{
    QJniObject name = QJniObject::fromString(portName);

    (void) AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getActivityClass(),
        "getDeviceId",
        "(Ljava/lang/String;)I",
        name.object<jstring>()
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

int getDeviceHandle(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getActivityClass(),
        "getDeviceHandle",
        "(I)I",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

int open(const QString &portName, QSerialPortPrivate *classPtr)
{
    QJniObject name = QJniObject::fromString(portName);

    (void) AndroidInterface::cleanJavaException();
    const int deviceId = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getActivityClass(),
        "open",
        "(Landroid/content/Context;Ljava/lang/String;J)I",
        QNativeInterface::QAndroidApplication::context(),
        name.object<jstring>(),
        reinterpret_cast<jlong>(classPtr)
    );
    (void) AndroidInterface::cleanJavaException();

    return deviceId;
}

bool close(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "close",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool isOpen(const QString &portName)
{
    QJniObject name = QJniObject::fromString(portName);

    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "isDeviceNameOpen",
        "(Ljava/lang/String;)Z",
        name.object<jstring>()
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

QByteArray read(int deviceId, int length, int timeout)
{
    (void) AndroidInterface::cleanJavaException();
    const QJniObject result = QJniObject::callStaticMethod<jbyteArray>(
        AndroidInterface::getActivityClass(),
        "read",
        "(II)[B",
        deviceId,
        length,
        timeout
    );
    (void) AndroidInterface::cleanJavaException();

    jbyteArray jarray = result.object<jbyteArray>();
    QJniEnvironment jniEnv;
    jbyte* const bytes = jniEnv->GetByteArrayElements(jarray, nullptr);
    const jsize len = jniEnv->GetArrayLength(jarray);
    QByteArray data = QByteArray::fromRawData(reinterpret_cast<char*>(bytes), len);
    jniEnv->ReleaseByteArrayElements(jarray, bytes, JNI_ABORT);
    (void) jniEnv.checkAndClearExceptions();

    return data;
}

int write(int deviceId, QByteArrayView data, int length, int timeout, bool async)
{
    QJniEnvironment jniEnv;
    jbyteArray jarray = jniEnv->NewByteArray(static_cast<jsize>(length));
    jniEnv->SetByteArrayRegion(jarray, 0, static_cast<jsize>(length), reinterpret_cast<const jbyte*>(data.constData()));

    (void) jniEnv.checkAndClearExceptions();
    timeout = qMax(0, timeout);
    int result;
    if (async) {
        result = QJniObject::callStaticMethod<jint>(
            AndroidInterface::getActivityClass(),
            "writeAsync",
            "(I[B)V",
            deviceId,
            jarray,
            timeout
        );
    } else {
        result = QJniObject::callStaticMethod<jint>(
            AndroidInterface::getActivityClass(),
            "write",
            "(I[BII)I",
            deviceId,
            jarray,
            length,
            timeout
        );
    }
    jniEnv->DeleteLocalRef(jarray);
    (void) jniEnv.checkAndClearExceptions();

    return result;
}

bool setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "setParameters",
        "(IIIII)Z",
        deviceId,
        baudRate,
        dataBits,
        stopBits,
        parity
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool getCarrierDetect(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "getCarrierDetect",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool getClearToSend(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "getClearToSend",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool getDataSetReady(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "getDataSetReady",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool getDataTerminalReady(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "getDataTerminalReady",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool setDataTerminalReady(int deviceId, bool set)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "setDataTerminalReady",
        "(IZ)Z",
        deviceId,
        set
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool getRingIndicator(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "getRingIndicator",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool getRequestToSend(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "getRequestToSend",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool setRequestToSend(int deviceId, bool set)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "setRequestToSend",
        "(IZ)Z",
        deviceId,
        set
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

QSerialPort::PinoutSignals getControlLines(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const QJniObject result = QJniObject::callStaticMethod<jintArray>(
        AndroidInterface::getActivityClass(),
        "getControlLines",
        "(I)[I",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    jintArray jarray = result.object<jintArray>();
    QJniEnvironment jniEnv;
    jint* const ints = jniEnv->GetIntArrayElements(jarray, nullptr);
    const jsize len = jniEnv->GetArrayLength(jarray);

    QSerialPort::PinoutSignals data = QSerialPort::PinoutSignals::fromInt(0);
    for (jsize i = 0; i < len; i++) {
        const jint value = ints[i];
        if ((value < ControlLine::RtsControlLine) || (value > ControlLine::RiControlLine)) {
            continue;
        }

        const ControlLine line = static_cast<ControlLine>(value);
        switch (line) {
        case ControlLine::RtsControlLine:
            (void) data.setFlag(QSerialPort::PinoutSignal::RequestToSendSignal, true);
            break;
        case ControlLine::CtsControlLine:
            (void) data.setFlag(QSerialPort::PinoutSignal::ClearToSendSignal, true);
            break;
        case ControlLine::DtrControlLine:
            (void) data.setFlag(QSerialPort::PinoutSignal::DataTerminalReadySignal, true);
            break;
        case ControlLine::DsrControlLine:
            (void) data.setFlag(QSerialPort::PinoutSignal::DataSetReadySignal, true);
            break;
        case ControlLine::CdControlLine:
            (void) data.setFlag(QSerialPort::PinoutSignal::DataCarrierDetectSignal, true);
            break;
        case ControlLine::RiControlLine:
            (void) data.setFlag(QSerialPort::PinoutSignal::RingIndicatorSignal, true);
            break;
        default:
            break;
        }
    }
    jniEnv->ReleaseIntArrayElements(jarray, ints, JNI_ABORT);
    (void) jniEnv.checkAndClearExceptions();

    return data;
}

int getFlowControl(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getActivityClass(),
        "getFlowControl",
        "(I)I",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return static_cast<QSerialPort::FlowControl>(result);
}

bool setFlowControl(int deviceId, int flowControl)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "setFlowControl",
        "(II)Z",
        deviceId,
        flowControl
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool purgeBuffers(int deviceId, bool input, bool output)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "purgeBuffers",
        "(IZZ)Z",
        deviceId,
        input,
        output
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool setBreak(int deviceId, bool set)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "setBreak",
        "(IZ)Z",
        deviceId,
        set
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool startReadThread(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "startIoManager",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool stopReadThread(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "stopIoManager",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

bool readThreadRunning(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const bool result = QJniObject::callStaticMethod<jboolean>(
        AndroidInterface::getActivityClass(),
        "ioManagerRunning",
        "(I)Z",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

int getReadBufferSize(int deviceId)
{
    (void) AndroidInterface::cleanJavaException();
    const int result = QJniObject::callStaticMethod<jint>(
        AndroidInterface::getActivityClass(),
        "ioManagerReadBufferSize",
        "(I)I",
        deviceId
    );
    (void) AndroidInterface::cleanJavaException();

    return result;
}

} // namespace AndroidSerial
