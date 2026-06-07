// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLISTVIEW_P_H
#define QQUICKLISTVIEW_P_H

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

QT_REQUIRE_CONFIG(quick_listview);

#include "qquickitemview_p.h"

#include <private/qtquickglobal_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickListView;
class QQuickListViewPrivate;
class Q_QUICK_EXPORT QQuickViewSection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(SectionCriteria criteria READ criteria WRITE setCriteria NOTIFY criteriaChanged)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int labelPositioning READ labelPositioning WRITE setLabelPositioning NOTIFY labelPositioningChanged)
    QML_NAMED_ELEMENT(ViewSection)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickViewSection(QQuickListView *parent=nullptr);

    QString property() const { return m_property; }
    void setProperty(const QString &);

    enum SectionCriteria { FullString, FirstCharacter };
    Q_ENUM(SectionCriteria)
    SectionCriteria criteria() const { return m_criteria; }
    void setCriteria(SectionCriteria);

    QQmlComponent *delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent *delegate);

    QString sectionString(const QString &value);

    enum LabelPositioning { InlineLabels = 0x01, CurrentLabelAtStart = 0x02, NextLabelAtEnd = 0x04 };
    Q_ENUM(LabelPositioning)
    int labelPositioning() const { return m_labelPositioning; }
    void setLabelPositioning(int pos);

Q_SIGNALS:
    void sectionsChanged();
    void propertyChanged();
    void criteriaChanged();
    void delegateChanged();
    void labelPositioningChanged();

private:
    QString m_property;
    SectionCriteria m_criteria;
    QQmlComponent *m_delegate;
    int m_labelPositioning;
    QQuickListViewPrivate *m_view;
};


class QQmlInstanceModel;
class QQuickListViewAttached;
class Q_QUICK_EXPORT QQuickListView : public QQuickItemView
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickListView)

    Q_PROPERTY(qreal highlightMoveVelocity READ highlightMoveVelocity WRITE setHighlightMoveVelocity NOTIFY highlightMoveVelocityChanged)
    Q_PROPERTY(qreal highlightResizeVelocity READ highlightResizeVelocity WRITE setHighlightResizeVelocity NOTIFY highlightResizeVelocityChanged)
    Q_PROPERTY(int highlightResizeDuration READ highlightResizeDuration WRITE setHighlightResizeDuration NOTIFY highlightResizeDurationChanged)

    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)

    Q_PROPERTY(QQuickViewSection *section READ sectionCriteria CONSTANT)
    Q_PROPERTY(QString currentSection READ currentSection NOTIFY currentSectionChanged)

    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged)

    Q_PROPERTY(HeaderPositioning headerPositioning READ headerPositioning WRITE setHeaderPositioning NOTIFY headerPositioningChanged REVISION(2, 4))
    Q_PROPERTY(FooterPositioning footerPositioning READ footerPositioning WRITE setFooterPositioning NOTIFY footerPositioningChanged REVISION(2, 4))

    Q_CLASSINFO("DefaultProperty", "data")
    QML_NAMED_ELEMENT(ListView)
    QML_ADDED_IN_VERSION(2, 0)
    QML_ATTACHED(QQuickListViewAttached)

public:
    QQuickListView(QQuickItem *parent=nullptr);
    ~QQuickListView();

    qreal spacing() const;
    void setSpacing(qreal spacing);

    enum Orientation { Horizontal = Qt::Horizontal, Vertical = Qt::Vertical };
    Q_ENUM(Orientation)
    Orientation orientation() const;
    void setOrientation(Orientation);

    QQuickViewSection *sectionCriteria();
    QString currentSection() const;

    void setHighlightFollowsCurrentItem(bool) override;

    qreal highlightMoveVelocity() const;
    void setHighlightMoveVelocity(qreal);

    qreal highlightResizeVelocity() const;
    void setHighlightResizeVelocity(qreal);

    int highlightResizeDuration() const;
    void setHighlightResizeDuration(int);

    void setHighlightMoveDuration(int) override;

    enum SnapMode { NoSnap, SnapToItem, SnapOneItem };
    Q_ENUM(SnapMode)
    SnapMode snapMode() const;
    void setSnapMode(SnapMode mode);

    enum HeaderPositioning { InlineHeader, OverlayHeader, PullBackHeader };
    Q_ENUM(HeaderPositioning)
    HeaderPositioning headerPositioning() const;
    void setHeaderPositioning(HeaderPositioning positioning);

    enum FooterPositioning { InlineFooter, OverlayFooter, PullBackFooter };
    Q_ENUM(FooterPositioning)
    FooterPositioning footerPositioning() const;
    void setFooterPositioning(FooterPositioning positioning);

    static QQuickListViewAttached *qmlAttachedProperties(QObject *);

public Q_SLOTS:
    void incrementCurrentIndex();
    void decrementCurrentIndex();

Q_SIGNALS:
    void spacingChanged();
    void orientationChanged();
    void currentSectionChanged();
    void highlightMoveVelocityChanged();
    void highlightResizeVelocityChanged();
    void highlightResizeDurationChanged();
    void snapModeChanged();
    Q_REVISION(2, 4) void headerPositioningChanged();
    Q_REVISION(2, 4) void footerPositioningChanged();

protected:
    void viewportMoved(Qt::Orientations orient) override;
    void keyPressEvent(QKeyEvent *) override;
    void geometryChange(const QRectF &newGeometry,const QRectF &oldGeometry) override;
    void initItem(int index, QObject *item) override;
    qreal maxYExtent() const override;
    qreal maxXExtent() const override;
};

class Q_QUICK_EXPORT QQuickListViewAttached : public QQuickItemViewAttached
{
    Q_OBJECT
    Q_PROPERTY(QQuickListView *view READ view NOTIFY viewChanged FINAL)
public:
    QQuickListViewAttached(QObject *parent)
        : QQuickItemViewAttached(parent), m_sectionItem(nullptr) {}
    ~QQuickListViewAttached() {}
    QQuickListView *view() const { return m_view; }
    void setView(QQuickListView *view) {
        if (view != m_view) {
            m_view = view;
            Q_EMIT viewChanged();
        }
    }

public:
    QPointer<QQuickItem> m_sectionItem;
private:
    QPointer<QQuickListView> m_view;
};


QT_END_NAMESPACE

#endif // QQUICKLISTVIEW_P_H
