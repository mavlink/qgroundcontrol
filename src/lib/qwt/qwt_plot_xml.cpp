/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot.h"

void QwtPlot::applyProperties(const QString &xmlDocument)
{
#if 1
    // Temporary dummy code, for designer tests
    setTitle(xmlDocument);
    replot();
#endif
}

QString QwtPlot::grabProperties() const
{
#if 1
    // Temporary dummy code, for designer tests
    return title().text();
#endif
}   
