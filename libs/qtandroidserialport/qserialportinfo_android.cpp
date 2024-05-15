// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport_p.h"

#include <QtCore/QStringList>
#include <QtCore/QJniObject>
#include <QtCore/QJniEnvironment>

#include <QGCLoggingCategory.h>

QGC_LOGGING_CATEGORY(QSerialPortInfo_AndroidLog, "qgc.libs.qtandroidserialport.qserialportinfo_android")

static constexpr const char* jniClassName = "org/mavlink/qgroundcontrol/QGCActivity";

QT_BEGIN_NAMESPACE

QList<QSerialPortInfo> availablePortsByFiltersOfDevices(bool &ok)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        ok = false;
        qCWarning(QSerialPortInfo_AndroidLog) << "Invalid QJniEnvironment";
        return QList<QSerialPortInfo>();
    }

    if (!QJniObject::isClassAvailable(jniClassName)) {
        ok = false;
        qCWarning(QSerialPortInfo_AndroidLog) << "Class Not Available";
        return QList<QSerialPortInfo>();
    }

    jclass javaClass = env.findClass(jniClassName);
    if (!javaClass) {
        ok = false;
        qCWarning(QSerialPortInfo_AndroidLog) << "Class Not Found";
        return QList<QSerialPortInfo>();
    }

    jmethodID methodId = env.findStaticMethod(javaClass, "availableDevicesInfo", "()[Ljava/lang/String;");
    if (!methodId) {
        ok = false;
        qCWarning(QSerialPortInfo_AndroidLog) << "Method Not Found";
        return QList<QSerialPortInfo>();
    }

    const QJniObject result = QJniObject::callStaticObjectMethod(javaClass, methodId);
    if (!result.isValid()) {
        ok = false;
        qCDebug(QSerialPortInfo_AndroidLog) << "Method Call Failed";
        return QList<QSerialPortInfo>();
    }

    QList<QSerialPortInfo> serialPortInfoList;

    const jobjectArray objArray = result.object<jobjectArray>();
    const jsize count = env->GetArrayLength(objArray);

    for (jsize i = 0; i < count; i++) {
        jobject obj = env->GetObjectArrayElement(objArray, i);
        jstring string = static_cast<jstring>(obj);
        const char *rawString = env->GetStringUTFChars(string, 0);
        qCDebug(QSerialPortInfo_AndroidLog) << "Adding device" << rawString;

        const QStringList strList = QString::fromUtf8(rawString).split(QStringLiteral(":"));
        env->ReleaseStringUTFChars(string, rawString);
        env->DeleteLocalRef(string);

        if (strList.size() < 4) {
            qCWarning(QSerialPortInfo_AndroidLog) << "Invalid device info";
            continue;
        }

        QSerialPortInfoPrivate priv;

        priv.portName             = strList.at(0);
        priv.device               = strList.at(0);
        // priv.description          = strList.at();
        priv.manufacturer         = strList.at(1);
        // priv.serialNumber         = strList.at();
        priv.productIdentifier    = strList.at(2).toInt();
        priv.hasProductIdentifier = (priv.productIdentifier != 0);
        priv.vendorIdentifier     = strList.at(3).toInt();
        priv.hasVendorIdentifier  = (priv.vendorIdentifier != 0);

        serialPortInfoList.append(priv);
    }

    ok = !serialPortInfoList.isEmpty();
    return serialPortInfoList;
}

QList<QSerialPortInfo> availablePortsBySysfs(bool &ok)
{
    ok = false;
    return QList<QSerialPortInfo>();
}

QList<QSerialPortInfo> availablePortsByUdev(bool &ok)
{
    ok = false;
    return QList<QSerialPortInfo>();
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    bool ok = false;
    const QList<QSerialPortInfo> serialPortInfoList = availablePortsByFiltersOfDevices(ok);

    if (ok) {
        return serialPortInfoList;
    } else {
        return QList<QSerialPortInfo>();
    }
}

QString QSerialPortInfoPrivate::portNameToSystemLocation(const QString &source)
{
    return (source.startsWith(QLatin1Char('/'))
            || source.startsWith(QLatin1String("./"))
            || source.startsWith(QLatin1String("../")))
            ? source : (QLatin1String("/dev/") + source);
}

QString QSerialPortInfoPrivate::portNameFromSystemLocation(const QString &source)
{
    return source.startsWith(QLatin1String("/dev/"))
            ? source.mid(5) : source;
}

QT_END_NAMESPACE
