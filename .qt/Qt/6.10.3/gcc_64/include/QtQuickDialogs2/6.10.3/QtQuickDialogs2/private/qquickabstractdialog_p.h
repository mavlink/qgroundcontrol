// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKABSTRACTDIALOG_P_H
#define QQUICKABSTRACTDIALOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <memory>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qobject.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/qpa/qplatformdialoghelper.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqml.h>
#include <QtQuickDialogs2Utils/private/qquickdialogtype_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>

#include "qtquickdialogs2global_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcDialogs)

class QWindow;
class QPlatformDialogHelper;

class Q_QUICKDIALOGS2_EXPORT QQuickAbstractDialog : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data FINAL)
    Q_PROPERTY(QWindow *parentWindow READ parentWindow WRITE setParentWindow NOTIFY parentWindowChanged RESET resetParentWindow FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(Qt::WindowFlags flags READ flags WRITE setFlags NOTIFY flagsChanged FINAL)
    Q_PROPERTY(Qt::WindowModality modality READ modality WRITE setModality NOTIFY modalityChanged FINAL)
    Q_PROPERTY(QQuickPopup::PopupType popupType READ popupType WRITE setPopupType RESET resetPopupType NOTIFY popupTypeChanged FINAL REVISION(6, 10))
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(int result READ result WRITE setResult NOTIFY resultChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "data")
    Q_MOC_INCLUDE(<QtGui/qwindow.h>)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 2)

public:
    explicit QQuickAbstractDialog(QQuickDialogType type, QObject *parent = nullptr);
    ~QQuickAbstractDialog();

    QPlatformDialogHelper *handle() const;

    QQmlListProperty<QObject> data();

    QWindow *parentWindow() const;
    void setParentWindow(QWindow *window);
    void resetParentWindow();

    QString title() const;
    void setTitle(const QString &title);

    Qt::WindowFlags flags() const;
    void setFlags(Qt::WindowFlags flags);

    Qt::WindowModality modality() const;
    void setModality(Qt::WindowModality modality);

    bool isVisible() const;
    void setVisible(bool visible);

    enum StandardCode { Rejected, Accepted };
    Q_ENUM(StandardCode)

    int result() const;
    void setResult(int result);

    QQuickPopup::PopupType popupType() const;
    void setPopupType(QQuickPopup::PopupType popupType);
    void resetPopupType();

public Q_SLOTS:
    void open();
    void close();
    virtual void accept();
    virtual void reject();
    virtual void done(int result);

Q_SIGNALS:
    void accepted();
    void rejected();
    void parentWindowChanged();
    void titleChanged();
    void flagsChanged();
    void modalityChanged();
    void visibleChanged();
    void resultChanged();
    Q_REVISION(6, 10) void popupTypeChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    enum class CreateOptions { TryAllDialogTypes = 0, DontTryNativeDialog = 1 };
    bool create(CreateOptions = CreateOptions::TryAllDialogTypes);
    void destroy();

    virtual bool useNativeDialog() const;
    virtual void onCreate(QPlatformDialogHelper *dialog);
    virtual void onShow(QPlatformDialogHelper *dialog);
    virtual void onHide(QPlatformDialogHelper *dialog);
    virtual int dialogCode() const;

    QQuickItem *findParentItem() const;
    QWindow *windowForOpen() const;
    void deferredOpen(QWindow *window);

    int m_result = Rejected;
    QWindow *m_parentWindow = nullptr;
    QString m_title;
    Qt::WindowFlags m_flags = Qt::Dialog;
    Qt::WindowModality m_modality = Qt::WindowModal;
    QQuickDialogType m_type = QQuickDialogType::FileDialog;
    QList<QObject *> m_data;
    std::unique_ptr<QPlatformDialogHelper> m_handle;
    QQuickPopup::PopupType m_popupType = QQuickPopup::Window;
    bool m_visibleRequested = false;
    bool m_visible = false;
    bool m_complete = false;
    bool m_parentWindowExplicitlySet = false;
    bool m_firstShow = true;
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTDIALOG_P_H
