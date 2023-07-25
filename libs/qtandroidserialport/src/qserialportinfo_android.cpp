/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
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

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport_android_p.h"

#include <QtCore/qscopedpointer.h>
#include <QtCore/qstringlist.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>

#include <android/log.h>

QT_BEGIN_NAMESPACE
static const char V_jniClassName[] {"org/mavlink/qgroundcontrol/QGCActivity"};
static const char V_TAG[] {"QGC_QSerialPortInfo"};

extern void cleanJavaException();

static int gErrorCount = 0;

QList<QSerialPortInfo> availablePortsByFiltersOfDevices(bool &ok)
{
    QList<QSerialPortInfo> serialPortInfoList;

    //__android_log_print(ANDROID_LOG_INFO, V_TAG, "Collecting device list");
    QAndroidJniObject resultL = QAndroidJniObject::callStaticObjectMethod(
        V_jniClassName,
        "availableDevicesInfo",
        "()[Ljava/lang/String;");
    
    if (!resultL.isValid()) {
        //-- If 5 consecutive errors, ignore it.
        if(gErrorCount < 5) {
            gErrorCount++;
            __android_log_print(ANDROID_LOG_ERROR, V_TAG, "Error from availableDevicesInfo");
        }
        ok = false;
        return serialPortInfoList;
    } else {
        gErrorCount = 0;
    }

    QAndroidJniEnvironment envL;
    jobjectArray objArrayL = resultL.object<jobjectArray>();
    int countL = envL->GetArrayLength(objArrayL);

    for (int iL = 0; iL < countL; iL++)
    {
        QSerialPortInfoPrivate priv;
        jstring stringL = (jstring)(envL->GetObjectArrayElement(objArrayL, iL));
        const char *rawStringL = envL->GetStringUTFChars(stringL, 0);
        //__android_log_print(ANDROID_LOG_INFO, V_TAG, "Adding device: %s", rawStringL);
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

QList<QSerialPortInfo> availablePortsBySysfs()
{
    bool ok;
    return availablePortsByFiltersOfDevices(ok);
}

QList<QSerialPortInfo> availablePortsByUdev()
{
    bool ok;
    return availablePortsByFiltersOfDevices(ok);
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    bool ok;
    return availablePortsByFiltersOfDevices(ok);
}

QList<qint32> QSerialPortInfo::standardBaudRates()
{
    return QSerialPortPrivate::standardBaudRates();
}

bool QSerialPortInfo::isBusy() const
{
    QAndroidJniObject jstrL = QAndroidJniObject::fromString(d_ptr->portName);
    cleanJavaException();
    jboolean resultL = QAndroidJniObject::callStaticMethod<jboolean>(
        V_jniClassName,
        "isDeviceNameOpen",
        "(Ljava/lang/String;)Z",
        jstrL.object<jstring>());
    cleanJavaException();
    return resultL;
}

bool QSerialPortInfo::isValid() const
{
    QAndroidJniObject jstrL = QAndroidJniObject::fromString(d_ptr->portName);
    cleanJavaException();
    jboolean resultL = QAndroidJniObject::callStaticMethod<jboolean>(
        V_jniClassName,
        "isDeviceNameValid",
        "(Ljava/lang/String;)Z",
        jstrL.object<jstring>());
    cleanJavaException();
    return resultL;
}

QString QSerialPortInfoPrivate::portNameToSystemLocation(const QString &source)
{
    return (source.startsWith(QLatin1Char('/'))
            || source.startsWith(QStringLiteral("./"))
            || source.startsWith(QStringLiteral("../")))
            ? source : (QStringLiteral("/dev/") + source);
}

QString QSerialPortInfoPrivate::portNameFromSystemLocation(const QString &source)
{
    return source.startsWith(QStringLiteral("/dev/"))
            ? source.mid(5) : source;
}

QT_END_NAMESPACE

