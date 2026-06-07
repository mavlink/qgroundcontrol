// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDIALOG_P_H
#define QDIALOG_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qwidget_p.h"
#include "QtCore/qeventloop.h"
#include "QtCore/qpointer.h"
#include "QtWidgets/qdialog.h"
#if QT_CONFIG(pushbutton)
#include "QtWidgets/qpushbutton.h"
#endif
#include <qpa/qplatformdialoghelper.h>

QT_REQUIRE_CONFIG(dialog);

QT_BEGIN_NAMESPACE

class QSizeGrip;

class Q_WIDGETS_EXPORT QDialogPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QDialog)
public:

    QDialogPrivate()
        :
#if QT_CONFIG(pushbutton)
          mainDef(nullptr),
#endif
          orientation(Qt::Horizontal),extension(nullptr), doShowExtension(false),
#if QT_CONFIG(sizegrip)
          resizer(nullptr),
          sizeGripEnabled(false),
#endif
          rescode(0), resetModalityTo(-1), wasModalitySet(true), eventLoop(nullptr),
          nativeDialogInUse(false), m_platformHelper(nullptr), m_platformHelperCreated(false)
        {}
    ~QDialogPrivate();

    void setVisible(bool visible) override;

    QWindow *transientParentWindow() const;
    bool setNativeDialogVisible(bool visible);
    QVariant styleHint(QPlatformDialogHelper::StyleHint hint) const;

#if QT_CONFIG(pushbutton)
    QPointer<QPushButton> mainDef;
#endif
    Qt::Orientation orientation;
    QWidget *extension;
    bool doShowExtension;
    QSize size, min, max;
#if QT_CONFIG(sizegrip)
    QSizeGrip *resizer;
    bool sizeGripEnabled;
#endif
    QPoint lastRMBPress;

#if QT_CONFIG(pushbutton)
    void setDefault(QPushButton *);
    void setMainDefault(QPushButton *);
    void hideDefault();
#endif
    void resetModalitySetByOpen();

    int rescode;
    int resetModalityTo;
    bool wasModalitySet;

    QPointer<QEventLoop> eventLoop;

    bool nativeDialogInUse;
    QPlatformDialogHelper *platformHelper() const;
    virtual bool canBeNativeDialog() const;

    void close(int resultCode);

protected:
    virtual int dialogCode() const { return rescode; }

private:
    virtual void initHelper(QPlatformDialogHelper *) {}
    virtual void helperPrepareShow(QPlatformDialogHelper *) {}
    virtual void helperDone(QDialog::DialogCode, QPlatformDialogHelper *) {}

    mutable QPlatformDialogHelper *m_platformHelper;
    mutable bool m_platformHelperCreated;
};

template <typename T>
class QAutoPointer {
    QPointer<T> o;
public:
    Q_NODISCARD_CTOR explicit QAutoPointer(T *t) noexcept : o(t) {}
    ~QAutoPointer() { delete o; }

    T *operator->() const noexcept { return get(); }
    T *get() const noexcept { return o; }
    T &operator*() const { return *get(); }
    explicit operator bool() const noexcept { return !o.isNull(); }
    bool operator!() const noexcept { return !o; }
private:
    Q_DISABLE_COPY(QAutoPointer);
};

QT_END_NAMESPACE

#endif // QDIALOG_P_H
