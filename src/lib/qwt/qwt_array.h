/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_ARRAY_H
#define QWT_ARRAY_H

#include "qwt_global.h"

/*!
  \def QwtArray
 */

#if QT_VERSION < 0x040000
#include <qmemarray.h>
#define QwtArray QMemArray
#else
#include <qvector.h>
#define QwtArray QVector
#endif

#endif
