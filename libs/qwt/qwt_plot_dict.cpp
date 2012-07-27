/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include "qwt_plot_dict.h"

class QwtPlotDict::PrivateData
{
public:

#if QT_VERSION < 0x040000
    class ItemList: public QValueList<QwtPlotItem *>
#else
    class ItemList: public QList<QwtPlotItem *>
#endif
    {
    public:
        void insertItem(QwtPlotItem *item) {
            if ( item == NULL )
                return;

            // Unfortunately there is no inSort operation
            // for lists in Qt4. The implementation below
            // is slow, but there shouldn't be many plot items.

#ifdef __GNUC__
#endif

#if QT_VERSION < 0x040000
            QValueListIterator<QwtPlotItem *> it;
#else
            QList<QwtPlotItem *>::Iterator it;
#endif
            for ( it = begin(); it != end(); ++it ) {
                if ( *it == item )
                    return;

                if ( (*it)->z() > item->z() ) {
                    insert(it, item);
                    return;
                }
            }
            append(item);
        }

        void removeItem(QwtPlotItem *item) {
            if ( item == NULL )
                return;

            int i = 0;

#if QT_VERSION < 0x040000
            QValueListIterator<QwtPlotItem *> it;
#else
            QList<QwtPlotItem *>::Iterator it;
#endif
            for ( it = begin(); it != end(); ++it ) {
                if ( item == *it ) {
#if QT_VERSION < 0x040000
                    remove(it);
#else
                    removeAt(i);
#endif
                    return;
                }
                i++;
            }
        }
    };

    ItemList itemList;
    bool autoDelete;
};

/*!
   Constructor

   Auto deletion is enabled.
   \sa setAutoDelete, attachItem
*/
QwtPlotDict::QwtPlotDict()
{
    d_data = new QwtPlotDict::PrivateData;
    d_data->autoDelete = true;
}

/*!
   Destructor

   If autoDelete is on, all attached items will be deleted
   \sa setAutoDelete, autoDelete, attachItem
*/
QwtPlotDict::~QwtPlotDict()
{
    detachItems(QwtPlotItem::Rtti_PlotItem, d_data->autoDelete);
    delete d_data;
}

/*!
   En/Disable Auto deletion

   If Auto deletion is on all attached plot items will be deleted
   in the destructor of QwtPlotDict. The default value is on.

   \sa autoDelete, attachItem
*/
void QwtPlotDict::setAutoDelete(bool autoDelete)
{
    d_data->autoDelete = autoDelete;
}

/*!
   \return true if auto deletion is enabled
   \sa setAutoDelete, attachItem
*/
bool QwtPlotDict::autoDelete() const
{
    return d_data->autoDelete;
}

/*!
   Attach/Detach a plot item

   Attached items will be deleted in the destructor,
   if auto deletion is enabled (default). Manually detached
   items are not deleted.

   \param item Plot item to attach/detach
   \ on If true attach, else detach the item

   \sa setAutoDelete, ~QwtPlotDict
*/
void QwtPlotDict::attachItem(QwtPlotItem *item, bool on)
{
    if ( on )
        d_data->itemList.insertItem(item);
    else
        d_data->itemList.removeItem(item);
}

/*!
   Detach items from the dictionary

   \param rtti In case of QwtPlotItem::Rtti_PlotItem detach all items
               otherwise only those items of the type rtti.
   \param autoDelete If true, delete all detached items
*/
void QwtPlotDict::detachItems(int rtti, bool autoDelete)
{
    PrivateData::ItemList list = d_data->itemList;
    QwtPlotItemIterator it = list.begin();
    while ( it != list.end() ) {
        QwtPlotItem *item = *it;

        ++it; // increment before removing item from the list

        if ( rtti == QwtPlotItem::Rtti_PlotItem || item->rtti() == rtti ) {
            item->attach(NULL);
            if ( autoDelete )
                delete item;
        }
    }
}

//! \brief A QwtPlotItemList of all attached plot items.
///
/// Use caution when iterating these lists, as removing/detaching an item will
/// invalidate the iterator. Instead you can place pointers to objects to be
/// removed in a removal list, and traverse that list later.
//! \return List of all attached plot items.
const QwtPlotItemList &QwtPlotDict::itemList() const
{
    return d_data->itemList;
}
