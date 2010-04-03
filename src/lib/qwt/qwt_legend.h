/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_LEGEND_H
#define QWT_LEGEND_H

#include <qframe.h>
#include "qwt_global.h"
#if QT_VERSION < 0x040000
#include <qvaluelist.h>
#else
#include <qlist.h>
#endif

class QScrollBar;
class QwtLegendItemManager;

/*!
  \brief The legend widget

  The QwtLegend widget is a tabular arrangement of legend items. Legend
  items might be any type of widget, but in general they will be
  a QwtLegendItem.

  \sa QwtLegendItem, QwtLegendItemManager QwtPlot
*/

class QWT_EXPORT QwtLegend : public QFrame
{
    Q_OBJECT

public:
    /*!
      \brief Display policy

       - NoIdentifier\n
         The client code is responsible how to display of each legend item.
         The Qwt library will not interfere.

       - FixedIdentifier\n
         All legend items are displayed with the QwtLegendItem::IdentifierMode
         to be passed in 'mode'.

       - AutoIdentifier\n
         Each legend item is displayed with a mode that is a bitwise or of
         - QwtLegendItem::ShowLine (if its curve is drawn with a line) and
         - QwtLegendItem::ShowSymbol (if its curve is drawn with symbols) and
         - QwtLegendItem::ShowText (if the has a title).

       Default is AutoIdentifier.
       \sa setDisplayPolicy(), displayPolicy(), QwtLegendItem::IdentifierMode
     */

    enum LegendDisplayPolicy
    {
        NoIdentifier = 0,
        FixedIdentifier = 1,
        AutoIdentifier = 2
    };

    //!  Interaction mode for the legend items
    enum LegendItemMode
    {
        ReadOnlyItem,
        ClickableItem,
        CheckableItem
    };

    explicit QwtLegend(QWidget *parent = NULL);
    virtual ~QwtLegend();
    
    void setDisplayPolicy(LegendDisplayPolicy policy, int mode);
    LegendDisplayPolicy displayPolicy() const;

    void setItemMode(LegendItemMode);
    LegendItemMode itemMode() const;

    int identifierMode() const;

    QWidget *contentsWidget();
    const QWidget *contentsWidget() const;

    void insert(const QwtLegendItemManager *, QWidget *);
    void remove(const QwtLegendItemManager *);

    QWidget *find(const QwtLegendItemManager *) const;
    QwtLegendItemManager *find(const QWidget *) const;

#if QT_VERSION < 0x040000
    virtual QValueList<QWidget *> legendItems() const;
#else
    virtual QList<QWidget *> legendItems() const;
#endif

    void clear();
    
    bool isEmpty() const;
    uint itemCount() const;

    virtual bool eventFilter(QObject *, QEvent *);

    virtual QSize sizeHint() const;
    virtual int heightForWidth(int w) const;

    QScrollBar *horizontalScrollBar() const;
    QScrollBar *verticalScrollBar() const;

protected:
    virtual void resizeEvent(QResizeEvent *);
    virtual void layoutContents();

private:
    class PrivateData;
    PrivateData *d_data;
};

#endif // QWT_LEGEND_H
