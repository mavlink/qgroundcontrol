/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qwidget.h>
#include "qwt_dyngrid_layout.h"
#include "qwt_math.h"

#if QT_VERSION < 0x040000
#include <qvaluelist.h>
#else
#include <qlist.h>
#endif

class QwtDynGridLayout::PrivateData
{
public:

#if QT_VERSION < 0x040000
    class LayoutIterator: public QGLayoutIterator
    {
    public:
        LayoutIterator(PrivateData *data):
            d_data(data)  
        {
            d_iterator = d_data->itemList.begin();
        }

        virtual QLayoutItem *current()
        { 
            if (d_iterator == d_data->itemList.end())
               return NULL;

            return *d_iterator;
        }

        virtual QLayoutItem *next()
        { 
            if (d_iterator == d_data->itemList.end())
               return NULL;

            d_iterator++;
            if (d_iterator == d_data->itemList.end())
               return NULL;

            return *d_iterator;
        }

        virtual QLayoutItem *takeCurrent()
        { 
            if ( d_iterator == d_data->itemList.end() )
                return NULL;

            QLayoutItem *item = *d_iterator;

            d_data->isDirty = true;
            d_iterator = d_data->itemList.remove(d_iterator);
            return item;
        }

    private:
        
        QValueListIterator<QLayoutItem*> d_iterator;
        QwtDynGridLayout::PrivateData *d_data;
    };
#endif

    PrivateData():
        isDirty(true)
    {
    }

#if QT_VERSION < 0x040000
    typedef QValueList<QLayoutItem*> LayoutItemList;
#else
    typedef QList<QLayoutItem*> LayoutItemList;
#endif

    mutable LayoutItemList itemList;

    uint maxCols;
    uint numRows;
    uint numCols;

#if QT_VERSION < 0x040000
    QSizePolicy::ExpandData expanding;
#else
    Qt::Orientations expanding;
#endif

    bool isDirty;
    QwtArray<QSize> itemSizeHints;
};


/*!
  \param parent Parent widget
  \param margin Margin
  \param spacing Spacing
*/

QwtDynGridLayout::QwtDynGridLayout(QWidget *parent, 
        int margin, int spacing):
    QLayout(parent)
{
    init();

    setSpacing(spacing);
    setMargin(margin);
}

#if QT_VERSION < 0x040000
/*!
  \param parent Parent widget
  \param spacing Spacing
*/
QwtDynGridLayout::QwtDynGridLayout(QLayout *parent, int spacing):
    QLayout(parent, spacing)
{
    init();
}
#endif

/*!
  \param spacing Spacing
*/

QwtDynGridLayout::QwtDynGridLayout(int spacing)
{
    init();
    setSpacing(spacing);
}

/*!
  Initialize the layout with default values.
*/
void QwtDynGridLayout::init()
{
    d_data = new QwtDynGridLayout::PrivateData;
    d_data->maxCols = d_data->numRows 
        = d_data->numCols = 0;

#if QT_VERSION < 0x040000
    d_data->expanding = QSizePolicy::NoDirection;
    setSupportsMargin(true);
#else
    d_data->expanding = 0;
#endif
}

//! Destructor

QwtDynGridLayout::~QwtDynGridLayout()
{
#if QT_VERSION < 0x040000
    deleteAllItems(); 
#endif

    delete d_data;
}

void QwtDynGridLayout::invalidate()
{
    d_data->isDirty = true;
    QLayout::invalidate();
}

void QwtDynGridLayout::updateLayoutCache()
{
    d_data->itemSizeHints.resize(itemCount());

    int index = 0;

    for (PrivateData::LayoutItemList::iterator it = d_data->itemList.begin();
        it != d_data->itemList.end(); ++it, index++)
    {
        d_data->itemSizeHints[int(index)] = (*it)->sizeHint();
    }

    d_data->isDirty = false;
}

/*!
  Limit the number of columns.
  \param maxCols upper limit, 0 means unlimited
  \sa QwtDynGridLayout::maxCols()
*/
  
void QwtDynGridLayout::setMaxCols(uint maxCols)
{
    d_data->maxCols = maxCols;
}

/*!
  Return the upper limit for the number of columns.
  0 means unlimited, what is the default.
  \sa QwtDynGridLayout::setMaxCols()
*/

uint QwtDynGridLayout::maxCols() const 
{ 
    return d_data->maxCols; 
}

//! Adds item to the next free position.

void QwtDynGridLayout::addItem(QLayoutItem *item)
{
    d_data->itemList.append(item);
    invalidate();
}

/*! 
  \return true if this layout is empty. 
*/

bool QwtDynGridLayout::isEmpty() const
{
    return d_data->itemList.isEmpty();
}

/*! 
  \return number of layout items
*/

uint QwtDynGridLayout::itemCount() const
{
    return d_data->itemList.count();
}

#if  QT_VERSION < 0x040000
/*! 
  \return An iterator over the children of this layout.
*/

QLayoutIterator QwtDynGridLayout::iterator()
{       
    return QLayoutIterator( 
        new QwtDynGridLayout::PrivateData::LayoutIterator(d_data) );
}

/*!
  Set whether this layout can make use of more space than sizeHint(). 
  A value of Vertical or Horizontal means that it wants to grow in only 
  one dimension, while BothDirections means that it wants to grow in 
  both dimensions. The default value is NoDirection. 
  \sa QwtDynGridLayout::expanding()
*/

void QwtDynGridLayout::setExpanding(QSizePolicy::ExpandData expanding)
{
    d_data->expanding = expanding;
}

/*!
  Returns whether this layout can make use of more space than sizeHint(). 
  A value of Vertical or Horizontal means that it wants to grow in only 
  one dimension, while BothDirections means that it wants to grow in 
  both dimensions. 
  \sa QwtDynGridLayout::setExpanding()
*/

QSizePolicy::ExpandData QwtDynGridLayout::expanding() const
{
    return d_data->expanding;
}

#else // QT_VERSION >= 0x040000

QLayoutItem *QwtDynGridLayout::itemAt( int index ) const
{
    if ( index < 0 || index >= d_data->itemList.count() )
        return NULL;

    return d_data->itemList.at(index);
}
    
QLayoutItem *QwtDynGridLayout::takeAt( int index )
{
    if ( index < 0 || index >= d_data->itemList.count() )
        return NULL;
  
    d_data->isDirty = true;
    return d_data->itemList.takeAt(index);
}

int QwtDynGridLayout::count() const
{
    return d_data->itemList.count();
}

void QwtDynGridLayout::setExpandingDirections(Qt::Orientations expanding)
{
    d_data->expanding = expanding;
}

Qt::Orientations QwtDynGridLayout::expandingDirections() const
{
    return d_data->expanding;
}

#endif

/*!
  Reorganizes columns and rows and resizes managed widgets within 
  the rectangle rect. 
*/

void QwtDynGridLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);

    if ( isEmpty() )
        return;

    d_data->numCols = columnsForWidth(rect.width());
    d_data->numRows = itemCount() / d_data->numCols;
    if ( itemCount() % d_data->numCols )
        d_data->numRows++;

#if QT_VERSION < 0x040000
    QValueList<QRect> itemGeometries = layoutItems(rect, d_data->numCols);
#else
    QList<QRect> itemGeometries = layoutItems(rect, d_data->numCols);
#endif

    int index = 0;
    for (PrivateData::LayoutItemList::iterator it = d_data->itemList.begin();
        it != d_data->itemList.end(); ++it)
    {
        QWidget *w = (*it)->widget();
        if ( w )
        {
            w->setGeometry(itemGeometries[index]);
            index++;
        }
    }
}

/*! 
  Calculate the number of columns for a given width. It tries to
  use as many columns as possible (limited by maxCols())

  \param width Available width for all columns
  \sa QwtDynGridLayout::maxCols(), QwtDynGridLayout::setMaxCols()
*/

uint QwtDynGridLayout::columnsForWidth(int width) const
{
    if ( isEmpty() )
        return 0;

    const int maxCols = (d_data->maxCols > 0) ? d_data->maxCols : itemCount();
    if ( maxRowWidth(maxCols) <= width )
        return maxCols;

    for (int numCols = 2; numCols <= maxCols; numCols++ )
    {
        const int rowWidth = maxRowWidth(numCols);
        if ( rowWidth > width )
            return numCols - 1;
    }

    return 1; // At least 1 column
}

/*! 
  Calculate the width of a layout for a given number of
  columns.

  \param numCols Given number of columns
  \param itemWidth Array of the width hints for all items
*/
int QwtDynGridLayout::maxRowWidth(int numCols) const
{
    int col;

    QwtArray<int> colWidth(numCols);
    for ( col = 0; col < (int)numCols; col++ )
        colWidth[col] = 0;

    if ( d_data->isDirty )
        ((QwtDynGridLayout*)this)->updateLayoutCache();

    for ( uint index = 0; 
        index < (uint)d_data->itemSizeHints.count(); index++ )
    {
        col = index % numCols;
        colWidth[col] = qwtMax(colWidth[col], 
            d_data->itemSizeHints[int(index)].width());
    }

    int rowWidth = 2 * margin() + (numCols - 1) * spacing();
    for ( col = 0; col < (int)numCols; col++ )
        rowWidth += colWidth[col];

    return rowWidth;
}

/*!
  \return the maximum width of all layout items
*/

int QwtDynGridLayout::maxItemWidth() const
{
    if ( isEmpty() )
        return 0;

    if ( d_data->isDirty )
        ((QwtDynGridLayout*)this)->updateLayoutCache();

    int w = 0;
    for ( uint i = 0; i < (uint)d_data->itemSizeHints.count(); i++ )
    {
        const int itemW = d_data->itemSizeHints[int(i)].width();
        if ( itemW > w )
            w = itemW;
    }

    return w;
}

/*!
  Calculate the geometries of the layout items for a layout
  with numCols columns and a given rect.
  \param rect Rect where to place the items
  \param numCols Number of columns
  \return item geometries
*/

#if QT_VERSION < 0x040000
QValueList<QRect> QwtDynGridLayout::layoutItems(const QRect &rect,
    uint numCols) const
#else
QList<QRect> QwtDynGridLayout::layoutItems(const QRect &rect,
    uint numCols) const
#endif
{
#if QT_VERSION < 0x040000
    QValueList<QRect> itemGeometries;
#else
    QList<QRect> itemGeometries;
#endif
    if ( numCols == 0 || isEmpty() )
        return itemGeometries;

    uint numRows = itemCount() / numCols;
    if ( numRows % itemCount() )
        numRows++;
 
    QwtArray<int> rowHeight(numRows);
    QwtArray<int> colWidth(numCols);
 
    layoutGrid(numCols, rowHeight, colWidth);

    bool expandH, expandV;
#if QT_VERSION >= 0x040000
    expandH = expandingDirections() & Qt::Horizontal;
    expandV = expandingDirections() & Qt::Vertical;
#else
    expandH = expanding() & QSizePolicy::Horizontally;
    expandV = expanding() & QSizePolicy::Vertically;
#endif

    if ( expandH || expandV )
        stretchGrid(rect, numCols, rowHeight, colWidth);

    QwtDynGridLayout *that = (QwtDynGridLayout *)this;
    const int maxCols = d_data->maxCols;
    that->d_data->maxCols = numCols;
    const QRect alignedRect = alignmentRect(rect);
    that->d_data->maxCols = maxCols;

    const int xOffset = expandH ? 0 : alignedRect.x();
    const int yOffset = expandV ? 0 : alignedRect.y();

    QwtArray<int> colX(numCols);
    QwtArray<int> rowY(numRows);

    const int xySpace = spacing();

    rowY[0] = yOffset + margin();
    for ( int r = 1; r < (int)numRows; r++ )
        rowY[r] = rowY[r-1] + rowHeight[r-1] + xySpace;

    colX[0] = xOffset + margin();
    for ( int c = 1; c < (int)numCols; c++ )
        colX[c] = colX[c-1] + colWidth[c-1] + xySpace;
    
    const int itemCount = d_data->itemList.size();
    for ( int i = 0; i < itemCount; i++ )
    {
        const int row = i / numCols;
        const int col = i % numCols;

        QRect itemGeometry(colX[col], rowY[row], 
            colWidth[col], rowHeight[row]);
        itemGeometries.append(itemGeometry);
    }

    return itemGeometries;
}


/*!
  Calculate the dimensions for the columns and rows for a grid
  of numCols columns.
  \param numCols Number of columns.
  \param rowHeight Array where to fill in the calculated row heights.
  \param colWidth Array where to fill in the calculated column widths.
*/

void QwtDynGridLayout::layoutGrid(uint numCols, 
    QwtArray<int>& rowHeight, QwtArray<int>& colWidth) const
{
    if ( numCols <= 0 )
        return;

    if ( d_data->isDirty )
        ((QwtDynGridLayout*)this)->updateLayoutCache();

    for ( uint index = 0; 
        index < (uint)d_data->itemSizeHints.count(); index++ )
    {
        const int row = index / numCols;
        const int col = index % numCols;

        const QSize &size = d_data->itemSizeHints[int(index)];

        rowHeight[row] = (col == 0) 
            ? size.height() : qwtMax(rowHeight[row], size.height());
        colWidth[col] = (row == 0) 
            ? size.width() : qwtMax(colWidth[col], size.width());
    }
}

/*!
  \return true: QwtDynGridLayout implements heightForWidth.
  \sa QwtDynGridLayout::heightForWidth()
*/

bool QwtDynGridLayout::hasHeightForWidth() const
{
    return true;
}

/*!
  \return The preferred height for this layout, given the width w. 
  \sa QwtDynGridLayout::hasHeightForWidth()
*/

int QwtDynGridLayout::heightForWidth(int width) const
{
    if ( isEmpty() )
        return 0;

    const uint numCols = columnsForWidth(width);
    uint numRows = itemCount() / numCols;
    if ( itemCount() % numCols )
        numRows++;

    QwtArray<int> rowHeight(numRows);
    QwtArray<int> colWidth(numCols);

    layoutGrid(numCols, rowHeight, colWidth);

    int h = 2 * margin() + (numRows - 1) * spacing();
    for ( int row = 0; row < (int)numRows; row++ )
        h += rowHeight[row];

    return h;
}

/*!
  Stretch columns in case of expanding() & QSizePolicy::Horizontal and
  rows in case of expanding() & QSizePolicy::Vertical to fill the entire
  rect. Rows and columns are stretched with the same factor.
  \sa QwtDynGridLayout::setExpanding(), QwtDynGridLayout::expanding()
*/

void QwtDynGridLayout::stretchGrid(const QRect &rect, 
    uint numCols, QwtArray<int>& rowHeight, QwtArray<int>& colWidth) const
{
    if ( numCols == 0 || isEmpty() )
        return;

    bool expandH, expandV;
#if QT_VERSION >= 0x040000
    expandH = expandingDirections() & Qt::Horizontal;
    expandV = expandingDirections() & Qt::Vertical;
#else
    expandH = expanding() & QSizePolicy::Horizontally;
    expandV = expanding() & QSizePolicy::Vertically;
#endif

    if ( expandH )
    {
        int xDelta = rect.width() - 2 * margin() - (numCols - 1) * spacing();
        for ( int col = 0; col < (int)numCols; col++ )
            xDelta -= colWidth[col];

        if ( xDelta > 0 )
        {
            for ( int col = 0; col < (int)numCols; col++ )
            {
                const int space = xDelta / (numCols - col);
                colWidth[col] += space;
                xDelta -= space;
            }
        }
    }

    if ( expandV )
    {
        uint numRows = itemCount() / numCols;
        if ( itemCount() % numCols )
            numRows++;

        int yDelta = rect.height() - 2 * margin() - (numRows - 1) * spacing();
        for ( int row = 0; row < (int)numRows; row++ )
            yDelta -= rowHeight[row];

        if ( yDelta > 0 )
        {
            for ( int row = 0; row < (int)numRows; row++ )
            {
                const int space = yDelta / (numRows - row);
                rowHeight[row] += space;
                yDelta -= space;
            }
        }
    }
}

/*!
   Return the size hint. If maxCols() > 0 it is the size for
   a grid with maxCols() columns, otherwise it is the size for
   a grid with only one row.
   \sa QwtDynGridLayout::maxCols(), QwtDynGridLayout::setMaxCols()
*/

QSize QwtDynGridLayout::sizeHint() const
{
    if ( isEmpty() )
        return QSize();

    const uint numCols = (d_data->maxCols > 0 ) ? d_data->maxCols : itemCount();
    uint numRows = itemCount() / numCols;
    if ( itemCount() % numCols )
        numRows++;

    QwtArray<int> rowHeight(numRows);
    QwtArray<int> colWidth(numCols);

    layoutGrid(numCols, rowHeight, colWidth);

    int h = 2 * margin() + (numRows - 1) * spacing();
    for ( int row = 0; row < (int)numRows; row++ )
        h += rowHeight[row];

    int w = 2 * margin() + (numCols - 1) * spacing(); 
    for ( int col = 0; col < (int)numCols; col++ )
        w += colWidth[col];

    return QSize(w, h);
}

/*!
  \return Number of rows of the current layout.
  \sa QwtDynGridLayout::numCols
  \warning The number of rows might change whenever the geometry changes
*/
uint QwtDynGridLayout::numRows() const 
{ 
    return d_data->numRows; 
}

/*!
  \return Number of columns of the current layout.
  \sa QwtDynGridLayout::numRows
  \warning The number of columns might change whenever the geometry changes
*/
uint QwtDynGridLayout::numCols() const 
{ 
    return d_data->numCols; 
}
