/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

/*! \file !*/
#ifndef QWT_PLOT_DICT
#define QWT_PLOT_DICT

#include "qwt_global.h"
#include "qwt_plot_item.h"

#if QT_VERSION < 0x040000
#include <qvaluelist.h>
typedef QValueListConstIterator<QwtPlotItem *> QwtPlotItemIterator;
/// \var typedef QValueList< QwtPlotItem *> QwtPlotItemList
/// \brief See QT 3.x assistant documentation for QValueList
typedef QValueList<QwtPlotItem *> QwtPlotItemList;
#else
#include <qlist.h>
typedef QList<QwtPlotItem *>::ConstIterator QwtPlotItemIterator;
/// \var typedef QList< QwtPlotItem *> QwtPlotItemList
/// \brief See QT 4.x assistant documentation for QList
typedef QList<QwtPlotItem *> QwtPlotItemList;
#endif

/*!
  \brief A dictionary for plot items

  QwtPlotDict organizes plot items in increasing z-order.
  If autoDelete() is enabled, all attached items will be deleted
  in the destructor of the dictionary.

  \sa QwtPlotItem::attach(), QwtPlotItem::detach(), QwtPlotItem::z()
*/
class QWT_EXPORT QwtPlotDict
{
public:
    explicit QwtPlotDict();
    ~QwtPlotDict();

    void setAutoDelete(bool);
    bool autoDelete() const;

    const QwtPlotItemList& itemList() const;

    void detachItems(int rtti = QwtPlotItem::Rtti_PlotItem,
        bool autoDelete = true);

private:
    friend class QwtPlotItem;

    void attachItem(QwtPlotItem *, bool);

    class PrivateData;
    PrivateData *d_data;
};

#endif
