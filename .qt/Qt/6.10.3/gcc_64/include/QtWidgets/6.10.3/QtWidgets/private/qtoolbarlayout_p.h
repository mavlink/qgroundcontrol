// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTOOLBARLAYOUT_P_H
#define QTOOLBARLAYOUT_P_H

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
#include <QtWidgets/qlayout.h>
#include <private/qlayoutengine_p.h>
#include <QList>

QT_REQUIRE_CONFIG(toolbar);

QT_BEGIN_NAMESPACE

class QAction;
class QToolBarExtension;
class QMenu;

class QToolBarItem : public QWidgetItem
{
public:
    QToolBarItem(QWidget *widget);
    bool isEmpty() const override;

    QAction *action;
    bool customWidget;
};

class Q_AUTOTEST_EXPORT QToolBarLayout : public QLayout
{
    Q_OBJECT

public:
    QToolBarLayout(QWidget *parent = nullptr);
    ~QToolBarLayout();

    void addItem(QLayoutItem *item) override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    int count() const override;

    bool isEmpty() const override;
    void invalidate() override;
    Qt::Orientations expandingDirections() const override;

    void setGeometry(const QRect &r) override;
    QSize minimumSize() const override;
    QSize sizeHint() const override;

    void insertAction(int index, QAction *action);
    int indexOf(const QAction *action) const;
    using QLayout::indexOf; // bring back the hidden members

    bool layoutActions(const QSize &size);
    QSize expandedSize(const QSize &size) const;
    bool expanded, animating;

    void setUsePopupMenu(bool set); // Yeah, there's no getter, but it's internal.
    void checkUsePopupMenu();

    bool movable() const;
    void updateMarginAndSpacing();
    bool hasExpandFlag() const;

    void updateMacBorderMetrics();
public Q_SLOTS:
    void setExpanded(bool b);

private:
    QList<QToolBarItem*> items;
    QSize hint, minSize;
    bool dirty, expanding, empty, expandFlag;
    QList<QLayoutStruct> geomArray;
    QRect handRect;
    QToolBarExtension *extension;

    void updateGeomArray() const;
    QToolBarItem *createItem(QAction *action);
    QMenu *popupMenu;
};

QT_END_NAMESPACE

#endif // QTOOLBARLAYOUT_P_H
