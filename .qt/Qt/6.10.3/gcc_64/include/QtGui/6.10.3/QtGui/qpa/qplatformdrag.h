// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMDRAG_H
#define QPLATFORMDRAG_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtGui/QPixmap>

QT_REQUIRE_CONFIG(draganddrop);

QT_BEGIN_NAMESPACE

class QMimeData;
class QMouseEvent;
class QDrag;
class QObject;
class QEvent;
class QPlatformDragPrivate;

class Q_GUI_EXPORT QPlatformDropQtResponse
{
public:
    QPlatformDropQtResponse(bool accepted, Qt::DropAction acceptedAction);
    bool isAccepted() const;
    Qt::DropAction acceptedAction() const;

private:
    bool m_accepted;
    Qt::DropAction m_accepted_action;

};

class Q_GUI_EXPORT QPlatformDragQtResponse : public QPlatformDropQtResponse
{
public:
    QPlatformDragQtResponse(bool accepted, Qt::DropAction acceptedAction, QRect answerRect);

    QRect answerRect() const;

private:
    QRect m_answer_rect;
};

class Q_GUI_EXPORT QPlatformDrag
{
    Q_DECLARE_PRIVATE(QPlatformDrag)
public:
    Q_DISABLE_COPY_MOVE(QPlatformDrag)

    QPlatformDrag();
    virtual ~QPlatformDrag();

    QDrag *currentDrag() const;

    virtual Qt::DropAction drag(QDrag *m_drag) = 0;
    virtual void cancelDrag();
    void updateAction(Qt::DropAction action);

    virtual Qt::DropAction defaultAction(Qt::DropActions possibleActions, Qt::KeyboardModifiers modifiers) const;

    static QPixmap defaultPixmap();

    virtual bool ownsDragObject() const;

private:
    QPlatformDragPrivate *d_ptr;
};

QT_END_NAMESPACE

#endif
