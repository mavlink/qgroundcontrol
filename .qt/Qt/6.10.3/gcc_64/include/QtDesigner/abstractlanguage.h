// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDESIGNER_ABTRACT_LANGUAGE_H
#define QDESIGNER_ABTRACT_LANGUAGE_H

#include <QtDesigner/extension.h>

QT_BEGIN_NAMESPACE

class QDialog;
class QWidget;
class QDesignerFormWindowInterface;
class QDesignerFormEditorInterface;
class QDesignerResourceBrowserInterface;

class QDesignerLanguageExtension
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerLanguageExtension)

    QDesignerLanguageExtension() = default;
    virtual ~QDesignerLanguageExtension() = default;

    /*!
        Returns the name to be matched against the "language" attribute of the <ui> element.

        \since 5.0
     */

    virtual QString name() const = 0;

    virtual QDialog *createFormWindowSettingsDialog(QDesignerFormWindowInterface *formWindow, QWidget *parentWidget) = 0;
    virtual QDesignerResourceBrowserInterface *createResourceBrowser(QWidget *parentWidget) = 0;

    virtual QDialog *createPromotionDialog(QDesignerFormEditorInterface *formEditor, QWidget *parentWidget = nullptr) = 0;

    virtual QDialog *createPromotionDialog(QDesignerFormEditorInterface *formEditor,
                                           const QString &promotableWidgetClassName,
                                           QString *promoteToClassName,
                                           QWidget *parentWidget = nullptr) = 0;

    virtual bool isLanguageResource(const QString &path) const = 0;

    virtual QString classNameOf(QObject *object) const = 0;

    virtual bool signalMatchesSlot(const QString &signal, const QString &slot) const = 0;

    virtual QString widgetBoxContents() const = 0;

    virtual QString uiExtension() const = 0;
};

Q_DECLARE_EXTENSION_INTERFACE(QDesignerLanguageExtension, "org.qt-project.Qt.Designer.Language.3")

QT_END_NAMESPACE

#endif // QDESIGNER_ABTRACT_LANGUAGE_H
