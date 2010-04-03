/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_DYNGRID_LAYOUT_H
#define QWT_DYNGRID_LAYOUT_H

#include <qlayout.h>
#include <qsize.h>
#if QT_VERSION >= 0x040000
#include <qlist.h>
#else
#include <qvaluelist.h>
#endif
#include "qwt_global.h"
#include "qwt_array.h"

/*!
  \brief The QwtDynGridLayout class lays out widgets in a grid,
         adjusting the number of columns and rows to the current size.
         
  QwtDynGridLayout takes the space it gets, divides it up into rows and 
  columns, and puts each of the widgets it manages into the correct cell(s). 
  It lays out as many number of columns as possible (limited by maxCols()).
*/

class QWT_EXPORT QwtDynGridLayout : public QLayout
{
    Q_OBJECT
public:
    explicit QwtDynGridLayout(QWidget *, int margin = 0, int space = -1);
#if QT_VERSION < 0x040000
    explicit QwtDynGridLayout(QLayout *, int space = -1);
#endif
    explicit QwtDynGridLayout(int space = -1);

    virtual ~QwtDynGridLayout();

    virtual void invalidate();

    void setMaxCols(uint maxCols);
    uint maxCols() const;

    uint numRows () const; 
    uint numCols () const;

    virtual void addItem(QLayoutItem *);

#if QT_VERSION >= 0x040000
    virtual QLayoutItem *itemAt( int index ) const;
    virtual QLayoutItem *takeAt( int index );
    virtual int count() const;

    void setExpandingDirections(Qt::Orientations);
    virtual Qt::Orientations expandingDirections() const;
    QList<QRect> layoutItems(const QRect &, uint numCols) const;
#else
    virtual QLayoutIterator iterator();

    void setExpanding(QSizePolicy::ExpandData);
    virtual QSizePolicy::ExpandData expanding() const;
    QValueList<QRect> layoutItems(const QRect &, uint numCols) const;
#endif

    virtual int maxItemWidth() const;

    virtual void setGeometry(const QRect &rect);

    virtual bool hasHeightForWidth() const;
    virtual int heightForWidth(int) const;

    virtual QSize sizeHint() const;

    virtual bool isEmpty() const;
    uint itemCount() const;

    virtual uint columnsForWidth(int width) const;

protected:

    void layoutGrid(uint numCols,
        QwtArray<int>& rowHeight, QwtArray<int>& colWidth) const;
    void stretchGrid(const QRect &rect, uint numCols, 
        QwtArray<int>& rowHeight, QwtArray<int>& colWidth) const;


private:
    void init();
    int maxRowWidth(int numCols) const;
    void updateLayoutCache();

#if QT_VERSION < 0x040000
// xlC 5.1, the IBM/AIX C++ compiler, needs it to be public
public:
#endif
   class PrivateData;

private:
    PrivateData *d_data;
};

#endif
