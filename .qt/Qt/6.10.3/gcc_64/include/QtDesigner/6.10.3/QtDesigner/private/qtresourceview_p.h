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

#ifndef QTRESOURCEVIEW_H
#define QTRESOURCEVIEW_H

#include "shared_global_p.h"
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qdialog.h>

QT_BEGIN_NAMESPACE

class QtResourceModel;
class QtResourceSet;
class QDesignerFormEditorInterface;
class QMimeData;

class QDESIGNER_SHARED_EXPORT QtResourceView : public QWidget
{
    Q_OBJECT
public:
    explicit QtResourceView(QDesignerFormEditorInterface *core, QWidget *parent = nullptr);
    ~QtResourceView();

    void setDragEnabled(bool dragEnabled);
    bool dragEnabled() const;

    QtResourceModel *model() const;
    void setResourceModel(QtResourceModel *model);

    QString selectedResource() const;
    void selectResource(const QString &resource);

    QString settingsKey() const;
    void setSettingsKey(const QString &key);

    bool isResourceEditingEnabled() const;
    void setResourceEditingEnabled(bool enable);

    // Helpers for handling the drag originating in QtResourceView (Xml/text)
    enum ResourceType { ResourceImage, ResourceStyleSheet, ResourceOther };
    static QString encodeMimeData(ResourceType resourceType, const QString &path);

    static bool decodeMimeData(const QMimeData *md, ResourceType *t = nullptr, QString *file = nullptr);
    static bool decodeMimeData(const QString &text, ResourceType *t = nullptr, QString *file = nullptr);

signals:
    void resourceSelected(const QString &resource);
    void resourceActivated(const QString &resource);

protected:
    bool event(QEvent *event) override;

private:
    QScopedPointer<class QtResourceViewPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QtResourceView)
    Q_DISABLE_COPY_MOVE(QtResourceView)
};

class QDESIGNER_SHARED_EXPORT  QtResourceViewDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QtResourceViewDialog(QDesignerFormEditorInterface *core, QWidget *parent = nullptr);
    ~QtResourceViewDialog() override;

    QString selectedResource() const;
    void selectResource(const QString &path);

    bool isResourceEditingEnabled() const;
    void setResourceEditingEnabled(bool enable);

private:
    QScopedPointer<class QtResourceViewDialogPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QtResourceViewDialog)
    Q_DISABLE_COPY_MOVE(QtResourceViewDialog)
};

QT_END_NAMESPACE

#endif
