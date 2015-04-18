/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
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

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport_android_p.h"

#include <QtCore/qscopedpointer.h>
#include <QtCore/qstringlist.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>

#include <android/log.h>

QT_BEGIN_NAMESPACE
static const char V_jniClassName[] {"org/qgroundcontrol/qgchelper/UsbDeviceJNI"};
static const char V_TAG[] {"QGC_QSerialPortInfo"};

QList<QSerialPortInfo> availablePortsByFiltersOfDevices()
{
    QList<QSerialPortInfo> serialPortInfoList;

    __android_log_print(ANDROID_LOG_INFO, V_TAG, "Collecting device list");
    QAndroidJniObject resultL = QAndroidJniObject::callStaticObjectMethod(
        V_jniClassName,
        "availableDevicesInfo",
        "()[Ljava/lang/String;");
    
    if (!resultL.isValid()) {
        __android_log_print(ANDROID_LOG_ERROR, V_TAG, "Error from availableDevicesInfo");
        return serialPortInfoList;
    }

    QAndroidJniEnvironment envL;
    jobjectArray objArrayL = resultL.object<jobjectArray>();
    int countL = envL->GetArrayLength(objArrayL);

    for (int iL=0; iL<countL; iL++)
    {
        QSerialPortInfo infoL;
        jstring stringL = (jstring)(envL->GetObjectArrayElement(objArrayL, iL));
        const char *rawStringL = envL->GetStringUTFChars(stringL, 0);
        __android_log_print(ANDROID_LOG_INFO, V_TAG, "Adding device: %s", rawStringL);
        QStringList strListL = QString::fromUtf8(rawStringL).split(QStringLiteral(":"));
        envL->ReleaseStringUTFChars(stringL, rawStringL);

        infoL.d_ptr->portName               = strListL[0];
        infoL.d_ptr->device                 = strListL[0];
        infoL.d_ptr->manufacturer           = strListL[1];
        infoL.d_ptr->productIdentifier      = strListL[2].toInt();
        infoL.d_ptr->hasProductIdentifier   = (infoL.d_ptr->productIdentifier != 0) ? true: false;
        infoL.d_ptr->vendorIdentifier       = strListL[3].toInt();
        infoL.d_ptr->hasVendorIdentifier    = (infoL.d_ptr->vendorIdentifier  != 0) ? true: false;

        serialPortInfoList.append(infoL);
    }

    return serialPortInfoList;
}

QList<QSerialPortInfo> availablePortsBySysfs()
{
    return availablePortsByFiltersOfDevices();
}

QList<QSerialPortInfo> availablePortsByUdev()
{
    return availablePortsByFiltersOfDevices();
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    return availablePortsByFiltersOfDevices();
}

QList<qint32> QSerialPortInfo::standardBaudRates()
{
    return QSerialPortPrivate::standardBaudRates();
}

bool QSerialPortInfo::isBusy() const
{
    QAndroidJniObject jstrL = QAndroidJniObject::fromString(d_ptr->portName);
    jboolean resultL = QAndroidJniObject::callStaticMethod<jboolean>(
        V_jniClassName,
        "isDeviceNameOpen",
        "(Ljava/lang/String;)Z",
        jstrL.object<jstring>());
    return resultL;
}

bool QSerialPortInfo::isValid() const
{
    QAndroidJniObject jstrL = QAndroidJniObject::fromString(d_ptr->portName);
    jboolean resultL = QAndroidJniObject::callStaticMethod<jboolean>(
        V_jniClassName,
        "isDeviceNameValid",
        "(Ljava/lang/String;)Z",
        jstrL.object<jstring>());
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

