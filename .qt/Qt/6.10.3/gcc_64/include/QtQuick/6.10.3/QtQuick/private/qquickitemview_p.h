// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEMVIEW_P_H
#define QQUICKITEMVIEW_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_itemview);

#include "qquickflickable_p.h"

#include <private/qqmldelegatemodel_p.h>

#include <qpointer.h>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcItemViewDelegateLifecycle)

class QQmlChangeSet;
class QQmlComponent;

class QQuickItemViewPrivate;

class Q_QUICK_EXPORT QQuickItemView : public QQuickFlickable
{
    Q_OBJECT

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QQuickItem *currentItem READ currentItem NOTIFY currentItemChanged)

    Q_PROPERTY(bool keyNavigationWraps READ isWrapEnabled WRITE setWrapEnabled NOTIFY keyNavigationWrapsChanged)
    Q_PROPERTY(bool keyNavigationEnabled READ isKeyNavigationEnabled WRITE setKeyNavigationEnabled NOTIFY keyNavigationEnabledChanged REVISION(2, 7))
    Q_PROPERTY(int cacheBuffer READ cacheBuffer WRITE setCacheBuffer NOTIFY cacheBufferChanged)
    Q_PROPERTY(int displayMarginBeginning READ displayMarginBeginning WRITE setDisplayMarginBeginning NOTIFY displayMarginBeginningChanged REVISION(2, 3))
    Q_PROPERTY(int displayMarginEnd READ displayMarginEnd WRITE setDisplayMarginEnd NOTIFY displayMarginEndChanged REVISION(2, 3))

    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged)
    Q_PROPERTY(Qt::LayoutDirection effectiveLayoutDirection READ effectiveLayoutDirection NOTIFY effectiveLayoutDirectionChanged)
    Q_PROPERTY(VerticalLayoutDirection verticalLayoutDirection READ verticalLayoutDirection WRITE setVerticalLayoutDirection NOTIFY verticalLayoutDirectionChanged)

    Q_PROPERTY(QQmlComponent *header READ header WRITE setHeader NOTIFY headerChanged)
    Q_PROPERTY(QQuickItem *headerItem READ headerItem NOTIFY headerItemChanged)
    Q_PROPERTY(QQmlComponent *footer READ footer WRITE setFooter NOTIFY footerChanged)
    Q_PROPERTY(QQuickItem *footerItem READ footerItem NOTIFY footerItemChanged)

#if QT_CONFIG(quick_viewtransitions)
    Q_PROPERTY(QQuickTransition *populate READ populateTransition WRITE setPopulateTransition NOTIFY populateTransitionChanged)
    Q_PROPERTY(QQuickTransition *add READ addTransition WRITE setAddTransition NOTIFY addTransitionChanged)
    Q_PROPERTY(QQuickTransition *addDisplaced READ addDisplacedTransition WRITE setAddDisplacedTransition NOTIFY addDisplacedTransitionChanged)
    Q_PROPERTY(QQuickTransition *move READ moveTransition WRITE setMoveTransition NOTIFY moveTransitionChanged)
    Q_PROPERTY(QQuickTransition *moveDisplaced READ moveDisplacedTransition WRITE setMoveDisplacedTransition NOTIFY moveDisplacedTransitionChanged)
    Q_PROPERTY(QQuickTransition *remove READ removeTransition WRITE setRemoveTransition NOTIFY removeTransitionChanged)
    Q_PROPERTY(QQuickTransition *removeDisplaced READ removeDisplacedTransition WRITE setRemoveDisplacedTransition NOTIFY removeDisplacedTransitionChanged)
    Q_PROPERTY(QQuickTransition *displaced READ displacedTransition WRITE setDisplacedTransition NOTIFY displacedTransitionChanged)
#endif

    Q_PROPERTY(QQmlComponent *highlight READ highlight WRITE setHighlight NOTIFY highlightChanged)
    Q_PROPERTY(QQuickItem *highlightItem READ highlightItem NOTIFY highlightItemChanged)
    Q_PROPERTY(bool highlightFollowsCurrentItem READ highlightFollowsCurrentItem WRITE setHighlightFollowsCurrentItem NOTIFY highlightFollowsCurrentItemChanged)
    Q_PROPERTY(HighlightRangeMode highlightRangeMode READ highlightRangeMode WRITE setHighlightRangeMode NOTIFY highlightRangeModeChanged)
    Q_PROPERTY(qreal preferredHighlightBegin READ preferredHighlightBegin WRITE setPreferredHighlightBegin NOTIFY preferredHighlightBeginChanged RESET resetPreferredHighlightBegin)
    Q_PROPERTY(qreal preferredHighlightEnd READ preferredHighlightEnd WRITE setPreferredHighlightEnd NOTIFY preferredHighlightEndChanged RESET resetPreferredHighlightEnd)
    Q_PROPERTY(int highlightMoveDuration READ highlightMoveDuration WRITE setHighlightMoveDuration NOTIFY highlightMoveDurationChanged)

    Q_PROPERTY(bool reuseItems READ reuseItems WRITE setReuseItems NOTIFY reuseItemsChanged REVISION(2, 15))
    Q_PROPERTY(QQmlDelegateModel::DelegateModelAccess delegateModelAccess READ delegateModelAccess
            WRITE setDelegateModelAccess NOTIFY delegateModelAccessChanged REVISION(6, 10) FINAL)

    QML_NAMED_ELEMENT(ItemView)
    QML_UNCREATABLE("ItemView is an abstract base class.")
    QML_ADDED_IN_VERSION(2, 1)

public:
    // this holds all layout enum values so they can be referred to by other enums
    // to ensure consistent values - e.g. QML references to GridView.TopToBottom flow
    // and GridView.TopToBottom vertical layout direction should have same value
    enum LayoutDirection {
        LeftToRight = Qt::LeftToRight,
        RightToLeft = Qt::RightToLeft,
        VerticalTopToBottom,
        VerticalBottomToTop
    };
    Q_ENUM(LayoutDirection)

    enum VerticalLayoutDirection {
        TopToBottom = VerticalTopToBottom,
        BottomToTop = VerticalBottomToTop
    };
    Q_ENUM(VerticalLayoutDirection)

    QQuickItemView(QQuickFlickablePrivate &dd, QQuickItem *parent = nullptr);
    ~QQuickItemView();

    QVariant model() const;
    void setModel(const QVariant &);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    int count() const;

    int currentIndex() const;
    void setCurrentIndex(int idx);

    QQuickItem *currentItem() const;

    bool isWrapEnabled() const;
    void setWrapEnabled(bool);

    bool isKeyNavigationEnabled() const;
    void setKeyNavigationEnabled(bool);

    int cacheBuffer() const;
    void setCacheBuffer(int);

    int displayMarginBeginning() const;
    void setDisplayMarginBeginning(int);

    int displayMarginEnd() const;
    void setDisplayMarginEnd(int);

    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection(Qt::LayoutDirection);
    Qt::LayoutDirection effectiveLayoutDirection() const;

    VerticalLayoutDirection verticalLayoutDirection() const;
    void setVerticalLayoutDirection(VerticalLayoutDirection layoutDirection);

    QQmlComponent *footer() const;
    void setFooter(QQmlComponent *);
    QQuickItem *footerItem() const;

    QQmlComponent *header() const;
    void setHeader(QQmlComponent *);
    QQuickItem *headerItem() const;

#if QT_CONFIG(quick_viewtransitions)
    QQuickTransition *populateTransition() const;
    void setPopulateTransition(QQuickTransition *transition);

    QQuickTransition *addTransition() const;
    void setAddTransition(QQuickTransition *transition);

    QQuickTransition *addDisplacedTransition() const;
    void setAddDisplacedTransition(QQuickTransition *transition);

    QQuickTransition *moveTransition() const;
    void setMoveTransition(QQuickTransition *transition);

    QQuickTransition *moveDisplacedTransition() const;
    void setMoveDisplacedTransition(QQuickTransition *transition);

    QQuickTransition *removeTransition() const;
    void setRemoveTransition(QQuickTransition *transition);

    QQuickTransition *removeDisplacedTransition() const;
    void setRemoveDisplacedTransition(QQuickTransition *transition);

    QQuickTransition *displacedTransition() const;
    void setDisplacedTransition(QQuickTransition *transition);
#endif

    QQmlComponent *highlight() const;
    void setHighlight(QQmlComponent *);

    QQuickItem *highlightItem() const;

    bool highlightFollowsCurrentItem() const;
    virtual void setHighlightFollowsCurrentItem(bool);

    enum HighlightRangeMode { NoHighlightRange, ApplyRange, StrictlyEnforceRange };
    Q_ENUM(HighlightRangeMode)
    HighlightRangeMode highlightRangeMode() const;
    void setHighlightRangeMode(HighlightRangeMode mode);

    qreal preferredHighlightBegin() const;
    void setPreferredHighlightBegin(qreal);
    void resetPreferredHighlightBegin();

    qreal preferredHighlightEnd() const;
    void setPreferredHighlightEnd(qreal);
    void resetPreferredHighlightEnd();

    int highlightMoveDuration() const;
    virtual void setHighlightMoveDuration(int);

    bool reuseItems() const;
    void setReuseItems(bool reuse);

    enum PositionMode { Beginning, Center, End, Visible, Contain, SnapPosition };
    Q_ENUM(PositionMode)

    Q_INVOKABLE void positionViewAtIndex(int index, int mode);
    Q_INVOKABLE int indexAt(qreal x, qreal y) const;
    Q_INVOKABLE QQuickItem *itemAt(qreal x, qreal y) const;
    Q_REVISION(2, 13) Q_INVOKABLE QQuickItem *itemAtIndex(int index) const;
    Q_INVOKABLE void positionViewAtBeginning();
    Q_INVOKABLE void positionViewAtEnd();
    Q_REVISION(2, 1) Q_INVOKABLE void forceLayout();

    void setContentX(qreal pos) override;
    void setContentY(qreal pos) override;
    qreal originX() const override;
    qreal originY() const override;

    QQmlDelegateModel::DelegateModelAccess delegateModelAccess() const;
    void setDelegateModelAccess(QQmlDelegateModel::DelegateModelAccess delegateModelAccess);

Q_SIGNALS:
    void modelChanged();
    void delegateChanged();
    void countChanged();
    void currentIndexChanged();
    void currentItemChanged();

    void keyNavigationWrapsChanged();
    Q_REVISION(2, 7) void keyNavigationEnabledChanged();
    void cacheBufferChanged();
    void displayMarginBeginningChanged();
    void displayMarginEndChanged();

    void layoutDirectionChanged();
    void effectiveLayoutDirectionChanged();
    void verticalLayoutDirectionChanged();

    void headerChanged();
    void footerChanged();
    void headerItemChanged();
    void footerItemChanged();

#if QT_CONFIG(quick_viewtransitions)
    void populateTransitionChanged();
    void addTransitionChanged();
    void addDisplacedTransitionChanged();
    void moveTransitionChanged();
    void moveDisplacedTransitionChanged();
    void removeTransitionChanged();
    void removeDisplacedTransitionChanged();
    void displacedTransitionChanged();
#endif

    void highlightChanged();
    void highlightItemChanged();
    void highlightFollowsCurrentItemChanged();
    void highlightRangeModeChanged();
    void preferredHighlightBeginChanged();
    void preferredHighlightEndChanged();
    void highlightMoveDurationChanged();

    Q_REVISION(2, 15) void reuseItemsChanged();
    Q_REVISION(6, 10) void delegateModelAccessChanged();

protected:
    void updatePolish() override;
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    qreal minYExtent() const override;
    qreal maxYExtent() const override;
    qreal minXExtent() const override;
    qreal maxXExtent() const override;

protected Q_SLOTS:
    void destroyRemoved();
    void createdItem(int index, QObject *item);
    virtual void initItem(int index, QObject *item);
    void modelUpdated(const QQmlChangeSet &changeSet, bool reset);
    void destroyingItem(QObject *item);
    Q_REVISION(2, 15) void onItemPooled(int modelIndex, QObject *object);
    Q_REVISION(2, 15) void onItemReused(int modelIndex, QObject *object);
    void animStopped();
    void trackedPositionChanged();

private:
    Q_DECLARE_PRIVATE(QQuickItemView)
};


class Q_QUICK_EXPORT QQuickItemViewAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isCurrentItem READ isCurrentItem NOTIFY currentItemChanged FINAL)
    Q_PROPERTY(bool delayRemove READ delayRemove WRITE setDelayRemove NOTIFY delayRemoveChanged FINAL)

    Q_PROPERTY(QString section READ section NOTIFY sectionChanged FINAL)
    Q_PROPERTY(QString previousSection READ prevSection NOTIFY prevSectionChanged FINAL)
    Q_PROPERTY(QString nextSection READ nextSection NOTIFY nextSectionChanged FINAL)

public:
    QQuickItemViewAttached(QObject *parent)
        : QObject(parent), m_isCurrent(false), m_delayRemove(false) {}
    ~QQuickItemViewAttached() {}

    bool isCurrentItem() const { return m_isCurrent; }
    void setIsCurrentItem(bool c) {
        if (m_isCurrent != c) {
            m_isCurrent = c;
            Q_EMIT currentItemChanged();
        }
    }

    bool delayRemove() const { return m_delayRemove; }
    void setDelayRemove(bool delay) {
        if (m_delayRemove != delay) {
            m_delayRemove = delay;
            Q_EMIT delayRemoveChanged();
        }
    }

    QString section() const { return m_section; }
    void setSection(const QString &sect) {
        if (m_section != sect) {
            m_section = sect;
            Q_EMIT sectionChanged();
        }
    }

    QString prevSection() const { return m_prevSection; }
    void setPrevSection(const QString &sect) {
        if (m_prevSection != sect) {
            m_prevSection = sect;
            Q_EMIT prevSectionChanged();
        }
    }

    QString nextSection() const { return m_nextSection; }
    void setNextSection(const QString &sect) {
        if (m_nextSection != sect) {
            m_nextSection = sect;
            Q_EMIT nextSectionChanged();
        }
    }

    void setSections(const QString &prev, const QString &sect, const QString &next) {
        bool prevChanged = prev != m_prevSection;
        bool sectChanged = sect != m_section;
        bool nextChanged = next != m_nextSection;
        m_prevSection = prev;
        m_section = sect;
        m_nextSection = next;
        if (prevChanged)
            Q_EMIT prevSectionChanged();
        if (sectChanged)
            Q_EMIT sectionChanged();
        if (nextChanged)
            Q_EMIT nextSectionChanged();
    }

    void emitAdd() { Q_EMIT add(); }
    void emitRemove() { Q_EMIT remove(); }

Q_SIGNALS:
    void viewChanged();
    void currentItemChanged();
    void delayRemoveChanged();

    void add();
    void remove();

    void sectionChanged();
    void prevSectionChanged();
    void nextSectionChanged();

    void pooled();
    void reused();

public:
    bool m_isCurrent : 1;
    bool m_delayRemove : 1;

    // current only used by list view
    mutable QString m_section;
    QString m_prevSection;
    QString m_nextSection;
};


QT_END_NAMESPACE

#endif // QQUICKITEMVIEW_P_H

