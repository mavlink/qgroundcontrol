/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_LEGEND_ITEM_MANAGER_H
#define QWT_LEGEND_ITEM_MANAGER_H

#include "qwt_global.h"

class QwtLegend;
class QWidget;

class QWT_EXPORT QwtLegendItemManager
{
public:
    QwtLegendItemManager() 
    {
    }

    virtual ~QwtLegendItemManager() 
    {
    }

    virtual void updateLegend(QwtLegend *) const = 0;
    virtual QWidget *legendItem() const = 0;
};

#endif

