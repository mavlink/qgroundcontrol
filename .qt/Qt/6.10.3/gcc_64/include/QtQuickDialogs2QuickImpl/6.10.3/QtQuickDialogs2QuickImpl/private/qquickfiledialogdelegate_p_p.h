// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFILEDIALOGDELEGATE_P_P_H
#define QQUICKFILEDIALOGDELEGATE_P_P_H

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

#include "qquickfiledialogimpl_p.h"
#include "qquickfiledialogimpl_p_p.h"
#include "qquickfolderdialogimpl_p.h"
#include "qquickfiledialogdelegate_p.h"
#include <QtQuick/private/qquicksinglepointhandler_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuick/private/qquickdroparea_p.h>
#include <QtQuickTemplates2/private/qquickitemdelegate_p_p.h>
#include <QtGui/qdrag.h>

QT_BEGIN_NAMESPACE

class QQuickFileDialogTapHandler : public QQuickTapHandler
{
    Q_OBJECT

public:
    explicit QQuickFileDialogTapHandler(QQuickItem *parent);

    enum State {
        Listening, // the pointer is not being pressed
        Tracking, // the pointer is being pressed
        DraggingStarted, // dragging started
        Dragging, // a drag is ongoing
        DraggingFinished // dragging finished
    };

    State state() { return m_state; }

    QQuickFileDialogImpl *getFileDialogImpl() const;
    void grabFolder();
    QUrl getFolderUrlAtPress() const;

    void handleDrag(QQuickDragEvent *event);
    void handleDrop(QQuickDragEvent *event);
    void handleContainsDragChanged();

    friend class QQuickFileDialogDelegatePrivate;

protected:
    void handleEventPoint(QPointerEvent *event, QEventPoint &point) override;
    bool wantsEventPoint(const QPointerEvent *event, const QEventPoint &point) override;

private:
    void resetDragData();

    QPointer<QDrag> m_drag;
    QPointer<QQuickDropArea> m_dropArea;
    State m_state = Listening;
    QUrl m_sourceUrl;
    bool m_longPressed = false;
};

class QQuickFileDialogDelegatePrivate : public QQuickItemDelegatePrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickFileDialogDelegate)

    static QQuickFileDialogDelegatePrivate *get(QQuickFileDialogDelegate *delegate)
    {
        return delegate->d_func();
    }

    void highlightFile();
    void chooseFile();
    void initTapHandler();
    void destroyTapHandler();
    void handleLongPress();

    bool acceptKeyClick(Qt::Key key) const override;

    QQuickDialog *dialog = nullptr;
    QQuickFileDialogImpl *fileDialog = nullptr;
    QQuickFolderDialogImpl *folderDialog = nullptr;
    QQuickFileDialogTapHandler *tapHandler = nullptr;
    QUrl file;
};

QT_END_NAMESPACE

#endif // QQUICKFILEDIALOGDELEGATE_P_P_H
