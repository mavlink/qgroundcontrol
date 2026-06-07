// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef EXTRAINFO_H
#define EXTRAINFO_H

#include <QtDesigner/sdk_global.h>
#include <QtDesigner/extension.h>

QT_BEGIN_NAMESPACE

class DomWidget;
class DomUI;
class QWidget;

class QDesignerFormEditorInterface;

class QDESIGNER_SDK_EXPORT QDesignerExtraInfoExtension
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerExtraInfoExtension)

    QDesignerExtraInfoExtension() = default;
    virtual ~QDesignerExtraInfoExtension() = default;

    virtual QDesignerFormEditorInterface *core() const = 0;
    virtual QWidget *widget() const = 0;

    virtual bool saveUiExtraInfo(DomUI *ui) = 0;
    virtual bool loadUiExtraInfo(DomUI *ui) = 0;

    virtual bool saveWidgetExtraInfo(DomWidget *ui_widget) = 0;
    virtual bool loadWidgetExtraInfo(DomWidget *ui_widget) = 0;

    QString workingDirectory() const;
    void setWorkingDirectory(const QString &workingDirectory);

private:
    QString m_workingDirectory;
};
Q_DECLARE_EXTENSION_INTERFACE(QDesignerExtraInfoExtension, "org.qt-project.Qt.Designer.ExtraInfo.2")

QT_END_NAMESPACE

#endif // EXTRAINFO_H
