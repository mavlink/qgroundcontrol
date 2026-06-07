// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFBWINDOW_P_H
#define QFBWINDOW_P_H

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

#include <qpa/qplatformwindow.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QFbBackingStore;
class QFbScreen;

class QFbWindow : public QPlatformWindow
{
public:
    QFbWindow(QWindow *window);
    ~QFbWindow();

    void raise() override;
    void lower() override;

    void setGeometry(const QRect &rect) override;
    void setVisible(bool visible) override;

    void setWindowState(Qt::WindowStates state) override;
    void setWindowFlags(Qt::WindowFlags type) override;
    Qt::WindowFlags windowFlags() const;

    WId winId() const override { return mWindowId; }

    void setBackingStore(QFbBackingStore *store) { mBackingStore = store; }
    QFbBackingStore *backingStore() const { return mBackingStore; }

    QFbScreen *platformScreen() const;

    virtual void repaint(const QRegion&);

    void propagateSizeHints() override { }
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool) override { return false; }

protected:
    friend class QFbScreen;

    QFbBackingStore *mBackingStore;
    QRect mOldGeometry;
    Qt::WindowFlags mWindowFlags;
    Qt::WindowStates mWindowState;

    WId mWindowId;
};

QT_END_NAMESPACE

#endif // QFBWINDOW_P_H

