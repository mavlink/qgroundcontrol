/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_VALUELIST_H
#define QWT_VALUELIST_H

#include "qwt_global.h"

/*!
  \def QwtValueList
 */

#if QT_VERSION < 0x040000

#include <qvaluelist.h>

#if defined(QWT_TEMPLATEDLL)
// MOC_SKIP_BEGIN
template class QWT_EXPORT QValueList<double>;
// MOC_SKIP_END
#endif

typedef QValueList<double> QwtValueList;

#else // QT_VERSION >= 0x040000

#include <qlist.h>

#if defined(QWT_TEMPLATEDLL)

#if QT_VERSION < 0x040300
// Some compilers have problems, 
// without a qHash(double) implementation
#include <qset.h>
#include <qvector.h>
inline uint qHash(double key) { return uint(key); }
#endif

// MOC_SKIP_BEGIN
template class QWT_EXPORT QList<double>;
// MOC_SKIP_END

#endif // QWT_TEMPLATEDLL

typedef QList<double> QwtValueList;

#endif

#endif
