// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTRESOURCEBROWSER_H
#define ABSTRACTRESOURCEBROWSER_H

#include <QtDesigner/sdk_global.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QWidget; // FIXME: fool syncqt

class QDESIGNER_SDK_EXPORT QDesignerResourceBrowserInterface : public QWidget
{
    Q_OBJECT
public:
    explicit QDesignerResourceBrowserInterface(QWidget *parent = nullptr);
    virtual ~QDesignerResourceBrowserInterface();

    virtual void setCurrentPath(const QString &filePath) = 0;
    virtual QString currentPath() const = 0;

Q_SIGNALS:
    void currentPathChanged(const QString &filePath);
    void pathActivated(const QString &filePath);
};

QT_END_NAMESPACE

#endif // ABSTRACTFORMEDITOR_H

