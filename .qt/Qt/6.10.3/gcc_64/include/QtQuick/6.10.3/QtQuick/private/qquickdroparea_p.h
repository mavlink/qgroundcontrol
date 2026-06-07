// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDROPAREA_P_H
#define QQUICKDROPAREA_P_H

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

#include <private/qtquickglobal_p.h>

#include <QtQuick/qquickitem.h>

#include <QtGui/qevent.h>

QT_REQUIRE_CONFIG(quick_draganddrop);

QT_BEGIN_NAMESPACE

class QQuickDropAreaPrivate;
class Q_QUICK_EXPORT QQuickDragEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x FINAL)
    Q_PROPERTY(qreal y READ y FINAL)
    Q_PROPERTY(QObject *source READ source FINAL)
    Q_PROPERTY(QStringList keys READ keys FINAL)
    Q_PROPERTY(Qt::DropActions supportedActions READ supportedActions FINAL)
    Q_PROPERTY(Qt::DropActions proposedAction READ proposedAction FINAL)
    Q_PROPERTY(Qt::DropAction action READ action WRITE setAction RESET resetAction FINAL)
    Q_PROPERTY(bool accepted READ accepted WRITE setAccepted FINAL)
    Q_PROPERTY(bool hasColor READ hasColor FINAL)
    Q_PROPERTY(bool hasHtml READ hasHtml FINAL)
    Q_PROPERTY(bool hasText READ hasText FINAL)
    Q_PROPERTY(bool hasUrls READ hasUrls FINAL)
    Q_PROPERTY(QVariant colorData READ colorData FINAL)
    Q_PROPERTY(QString html READ html FINAL)
    Q_PROPERTY(QString text READ text FINAL)
    Q_PROPERTY(QList<QUrl> urls READ urls FINAL)
    Q_PROPERTY(QStringList formats READ formats FINAL)
    QML_NAMED_ELEMENT(DragEvent)
    QML_UNCREATABLE("DragEvent is only meant to be created by DropArea")
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickDragEvent(QQuickDropAreaPrivate *d, QDropEvent *event) : d(d), event(event) {}

    qreal x() const { return event->position().x(); }
    qreal y() const { return event->position().y(); }

    QObject *source() const;

    Qt::DropActions supportedActions() const { return event->possibleActions(); }
    Qt::DropActions proposedAction() const { return event->proposedAction(); }
    Qt::DropAction action() const { return event->dropAction(); }
    void setAction(Qt::DropAction action) { event->setDropAction(action); }
    void resetAction() { event->setDropAction(event->proposedAction()); }

    QStringList keys() const;

    bool accepted() const { return event->isAccepted(); }
    void setAccepted(bool accepted) { event->setAccepted(accepted); }

    bool hasColor() const;
    bool hasHtml() const;
    bool hasText() const;
    bool hasUrls() const;
    QVariant colorData() const;
    QString html() const;
    QString text() const;
    QList<QUrl> urls() const;
    QStringList formats() const;

    Q_INVOKABLE QString getDataAsString(const QString &format) const;
    Q_INVOKABLE QByteArray getDataAsArrayBuffer(const QString &format) const;
    Q_INVOKABLE void acceptProposedAction();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void accept(Qt::DropAction action);

private:
    QQuickDropAreaPrivate *d;
    QDropEvent *event;
};

class QQuickDropAreaDrag : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal y READ y NOTIFY positionChanged FINAL)
    Q_PROPERTY(QObject *source READ source NOTIFY sourceChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickDropAreaDrag(QQuickDropAreaPrivate *d, QObject *parent = nullptr);
    ~QQuickDropAreaDrag();

    qreal x() const;
    qreal y() const;
    QObject *source() const;

Q_SIGNALS:
    void positionChanged();
    void sourceChanged();

private:
    QQuickDropAreaPrivate *d;

    friend class QQuickDropArea;
    friend class QQuickDropAreaPrivate;
};

class QQuickDropAreaPrivate;
class Q_QUICK_EXPORT QQuickDropArea : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool containsDrag READ containsDrag NOTIFY containsDragChanged)
    Q_PROPERTY(QStringList keys READ keys WRITE setKeys NOTIFY keysChanged)
    Q_PROPERTY(QQuickDropAreaDrag *drag READ drag CONSTANT)
    QML_NAMED_ELEMENT(DropArea)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickDropArea(QQuickItem *parent=0);
    ~QQuickDropArea();

    bool containsDrag() const;
    void setContainsDrag(bool drag);

    QStringList keys() const;
    void setKeys(const QStringList &keys);

    QQuickDropAreaDrag *drag();

Q_SIGNALS:
    void containsDragChanged();
    void keysChanged();
    void sourceChanged();

    void entered(QQuickDragEvent *drag);
    void exited();
    void positionChanged(QQuickDragEvent *drag);
    void dropped(QQuickDragEvent *drop);

protected:
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickDropArea)
    Q_DECLARE_PRIVATE(QQuickDropArea)
};

QT_END_NAMESPACE

#endif // QQUICKDROPAREA_P_H
