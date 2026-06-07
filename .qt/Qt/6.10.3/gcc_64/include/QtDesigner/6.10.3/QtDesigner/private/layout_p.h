// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef LAYOUT_H
#define LAYOUT_H

#include "shared_global_p.h"
#include "layoutinfo_p.h"

#include <QtCore/qpointer.h>
#include <QtCore/qobject.h>
#include <QtCore/qmap.h>
#include <QtCore/qhash.h>

#include <QtWidgets/qlayout.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;

namespace qdesigner_internal {
class QDESIGNER_SHARED_EXPORT Layout : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Layout)
protected:
    Layout(const QWidgetList &wl, QWidget *p, QDesignerFormWindowInterface *fw, QWidget *lb, LayoutInfo::Type layoutType);

public:
    static  Layout* createLayout(const QWidgetList &widgets,  QWidget *parentWidget,
                                 QDesignerFormWindowInterface *fw,
                                 QWidget *layoutBase, LayoutInfo::Type layoutType);

    ~Layout() override;

    virtual void sort() = 0;
    virtual void doLayout() = 0;

    virtual void setup();
    virtual void undoLayout();
    virtual void breakLayout();

    const QWidgetList &widgets() const { return m_widgets; }
    QWidget *parentWidget() const      { return m_parentWidget; }
    QWidget *layoutBaseWidget() const  { return m_layoutBase; }

    /* Determines whether instances of QLayoutWidget are unmanaged/hidden
     * after breaking a layout. Default is true. Can be turned off when
      * morphing */
    bool reparentLayoutWidget() const  { return m_reparentLayoutWidget; }
    void setReparentLayoutWidget(bool v) {  m_reparentLayoutWidget = v; }

protected:
    virtual void finishLayout(bool needMove, QLayout *layout = nullptr);
    virtual bool prepareLayout(bool &needMove, bool &needReparent);

    void setWidgets(const  QWidgetList &widgets) { m_widgets = widgets; }
    QLayout *createLayout(int type);
    void reparentToLayoutBase(QWidget *w);

private slots:
    void widgetDestroyed();

private:
    QWidgetList m_widgets;
    QWidget *m_parentWidget;
    QHash<QWidget *, QRect> m_geometries;
    QWidget *m_layoutBase;
    QDesignerFormWindowInterface *m_formWindow;
    const LayoutInfo::Type m_layoutType;
    QPoint m_startPoint;
    QRect m_oldGeometry;

    bool m_reparentLayoutWidget;
    const bool m_isBreak;
};

namespace Utils
{

inline int indexOfWidget(QLayout *layout, QWidget *widget)
{
    int index = 0;
    while (QLayoutItem *item = layout->itemAt(index)) {
        if (item->widget() == widget)
            return index;

        ++index;
    }

    return -1;
}

} // namespace Utils

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // LAYOUT_H
