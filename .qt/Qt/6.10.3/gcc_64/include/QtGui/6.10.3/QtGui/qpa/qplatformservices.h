// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMSERVICES_H
#define QPLATFORMSERVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcQpaServices, Q_GUI_EXPORT)

class QUrl;
class QWindow;

class Q_GUI_EXPORT QPlatformServiceColorPicker : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void pickColor() = 0;
Q_SIGNALS:
    void colorPicked(const QColor &color);
};

class Q_GUI_EXPORT QPlatformServices
{
public:
    Q_DISABLE_COPY_MOVE(QPlatformServices)

    enum Capability {
        ColorPicking,
    };

    QPlatformServices();
    virtual ~QPlatformServices() { }

    virtual bool openUrl(const QUrl &url);
    virtual bool openDocument(const QUrl &url);

    virtual QByteArray desktopEnvironment() const;

    virtual bool hasCapability(Capability capability) const;

    virtual QPlatformServiceColorPicker *colorPicker(QWindow *parent = nullptr);
};

QT_END_NAMESPACE

#endif // QPLATFORMSERVICES_H
