// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDRAG_P_H
#define QQUICKDRAG_P_H

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

#include <QtQuick/qquickitem.h>

#include <private/qintrusivelist_p.h>
#include <private/qqmlguard_p.h>
#include <private/qtquickglobal_p.h>

#include <QtCore/qmimedata.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qurl.h>
#include <QtGui/qevent.h>

QT_REQUIRE_CONFIG(quick_draganddrop);

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickDrag;

class QQuickDragGrabber
{
    class Item : public QQmlGuard<QQuickItem>
    {
    public:
        Item(QQuickItem *item) : QQmlGuard<QQuickItem>(Item::objectDestroyedImpl, item) {}

        QIntrusiveListNode node;
    private:
        static void objectDestroyedImpl(QQmlGuardImpl *guard) { delete static_cast<Item *>(guard); }
    };

    typedef QIntrusiveList<Item, &Item::node> ItemList;

public:
    QQuickDragGrabber() : m_target(nullptr) {}
    ~QQuickDragGrabber() { while (!m_items.isEmpty()) delete m_items.first(); }


    QObject *target() const
    {
        if (m_target)
            return m_target;
        else if (!m_items.isEmpty())
            return *m_items.first();
        else
            return nullptr;
    }
    void setTarget(QObject *target) { m_target = target; }
    void resetTarget() { m_target = nullptr; }

    bool isEmpty() const { return m_items.isEmpty(); }

    typedef ItemList::iterator iterator;
    iterator begin() { return m_items.begin(); }
    iterator end() { return m_items.end(); }

    void grab(QQuickItem *item) { m_items.insert(new Item(item)); }
    iterator release(iterator at) { Item *item = *at; at = at.erase(); delete item; return at; }

    auto& ignoreList() { return m_ignoreDragItems; }

private:

    ItemList m_items;
    QVarLengthArray<QQuickItem *, 4> m_ignoreDragItems;
    QObject *m_target;
};

class QQuickDropEventEx : public QDropEvent
{
public:
    void setProposedAction(Qt::DropAction action) { m_defaultAction = action; m_dropAction = action; }

    static void setProposedAction(QDropEvent *event, Qt::DropAction action) {
        static_cast<QQuickDropEventEx *>(event)->setProposedAction(action);
    }

    void copyActions(const QDropEvent &from) {
        m_defaultAction = from.proposedAction(); m_dropAction = from.dropAction(); }

    static void copyActions(QDropEvent *to, const QDropEvent &from) {
        static_cast<QQuickDropEventEx *>(to)->copyActions(from);
    }
};

class QQuickDragMimeData : public QMimeData
{
    Q_OBJECT
public:
    QQuickDragMimeData()
        : m_source(nullptr)
    {
    }

    QStringList keys() const { return m_keys; }
    QObject *source() const { return m_source; }

private:
    QObject *m_source;
    Qt::DropActions m_supportedActions;
    QStringList m_keys;

    friend class QQuickDragAttached;
    friend class QQuickDragAttachedPrivate;
};

class QQuickDragAttached;
class Q_QUICK_EXPORT QQuickDrag : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged RESET resetTarget FINAL)
    Q_PROPERTY(Axis axis READ axis WRITE setAxis NOTIFY axisChanged FINAL FINAL)
    Q_PROPERTY(qreal minimumX READ xmin WRITE setXmin NOTIFY minimumXChanged FINAL)
    Q_PROPERTY(qreal maximumX READ xmax WRITE setXmax NOTIFY maximumXChanged FINAL)
    Q_PROPERTY(qreal minimumY READ ymin WRITE setYmin NOTIFY minimumYChanged FINAL)
    Q_PROPERTY(qreal maximumY READ ymax WRITE setYmax NOTIFY maximumYChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool filterChildren READ filterChildren WRITE setFilterChildren NOTIFY filterChildrenChanged FINAL)
    Q_PROPERTY(bool smoothed READ smoothed WRITE setSmoothed NOTIFY smoothedChanged FINAL)
    // Note, threshold was added in QtQuick 2.2 but REVISION is not supported (or needed) for grouped
    // properties See QTBUG-33179
    Q_PROPERTY(qreal threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged RESET resetThreshold FINAL)
    //### consider drag and drop

    QML_NAMED_ELEMENT(Drag)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Drag is only available via attached properties.")
    QML_ATTACHED(QQuickDragAttached)

public:
    QQuickDrag(QObject *parent=nullptr);
    ~QQuickDrag();

    enum DragType { None, Automatic, Internal };
    Q_ENUM(DragType)

    QQuickItem *target() const;
    void setTarget(QQuickItem *target);
    void resetTarget();

    enum Axis { XAxis=0x01, YAxis=0x02, XAndYAxis=0x03, XandYAxis=XAndYAxis };
    Q_ENUM(Axis)
    Axis axis() const;
    void setAxis(Axis);

    qreal xmin() const;
    void setXmin(qreal);
    qreal xmax() const;
    void setXmax(qreal);
    qreal ymin() const;
    void setYmin(qreal);
    qreal ymax() const;
    void setYmax(qreal);

    bool smoothed() const;
    void setSmoothed(bool smooth);

    qreal threshold() const;
    void setThreshold(qreal);
    void resetThreshold();

    bool active() const;
    void setActive(bool);

    bool filterChildren() const;
    void setFilterChildren(bool);

    static QQuickDragAttached *qmlAttachedProperties(QObject *obj);

Q_SIGNALS:
    void targetChanged();
    void axisChanged();
    void minimumXChanged();
    void maximumXChanged();
    void minimumYChanged();
    void maximumYChanged();
    void activeChanged();
    void filterChildrenChanged();
    void smoothedChanged();
    void thresholdChanged();

private:
    QQuickItem *_target;
    Axis _axis;
    qreal _xmin;
    qreal _xmax;
    qreal _ymin;
    qreal _ymax;
    bool _active : 1;
    bool _filterChildren: 1;
    bool _smoothed : 1;
    qreal _threshold;
    Q_DISABLE_COPY(QQuickDrag)
};

class QQuickDragAttachedPrivate;
class Q_QUICK_EXPORT QQuickDragAttached : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickDragAttached)

    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QObject *source READ source WRITE setSource NOTIFY sourceChanged RESET resetSource FINAL)
    Q_PROPERTY(QObject *target READ target NOTIFY targetChanged FINAL)
    Q_PROPERTY(QPointF hotSpot READ hotSpot WRITE setHotSpot NOTIFY hotSpotChanged FINAL)
    Q_PROPERTY(QUrl imageSource READ imageSource WRITE setImageSource NOTIFY imageSourceChanged FINAL)
    // imageSourceSize is new in Qt 6.8; revision omitted because of QTBUG-33179
    Q_PROPERTY(QSize imageSourceSize READ imageSourceSize WRITE setImageSourceSize NOTIFY imageSourceSizeChanged FINAL)
    Q_PROPERTY(QStringList keys READ keys WRITE setKeys NOTIFY keysChanged FINAL)
    Q_PROPERTY(QVariantMap mimeData READ mimeData WRITE setMimeData NOTIFY mimeDataChanged FINAL)
    Q_PROPERTY(Qt::DropActions supportedActions READ supportedActions WRITE setSupportedActions NOTIFY supportedActionsChanged FINAL)
    Q_PROPERTY(Qt::DropAction proposedAction READ proposedAction WRITE setProposedAction NOTIFY proposedActionChanged FINAL)
    Q_PROPERTY(QQuickDrag::DragType dragType READ dragType WRITE setDragType NOTIFY dragTypeChanged FINAL)

    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickDragAttached(QObject *parent);
    ~QQuickDragAttached();

    bool isActive() const;
    void setActive(bool active);

    QObject *source() const;
    void setSource(QObject *item);
    void resetSource();

    QObject *target() const;

    QPointF hotSpot() const;
    void setHotSpot(const QPointF &hotSpot);

    QUrl imageSource() const;
    void setImageSource(const QUrl &url);

    QSize imageSourceSize() const;
    void setImageSourceSize(const QSize &size);

    QStringList keys() const;
    void setKeys(const QStringList &keys);

    QVariantMap mimeData() const;
    void setMimeData(const QVariantMap &mimeData);

    Qt::DropActions supportedActions() const;
    void setSupportedActions(Qt::DropActions actions);

    Qt::DropAction proposedAction() const;
    void setProposedAction(Qt::DropAction action);

    QQuickDrag::DragType dragType() const;
    void setDragType(QQuickDrag::DragType dragType);

    Q_INVOKABLE int drop();

    bool event(QEvent *event) override;

public Q_SLOTS:
    void start(QQmlV4FunctionPtr);
    void startDrag(QQmlV4FunctionPtr);
    void cancel();

Q_SIGNALS:
    void dragStarted();
    void dragFinished(Qt::DropAction dropAction);

    void activeChanged();
    void sourceChanged();
    void targetChanged();
    void hotSpotChanged();
    void imageSourceChanged();
    void imageSourceSizeChanged(); // new in Qt 6.8
    void keysChanged();
    void mimeDataChanged();
    void supportedActionsChanged();
    void proposedActionChanged();
    void dragTypeChanged();
};

QT_END_NAMESPACE

#endif
