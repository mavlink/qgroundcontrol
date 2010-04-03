/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include "qwt_math.h"

/*!
  \brief Find the smallest value in an array
  \param array Pointer to an array
  \param size Array size
*/
double qwtGetMin(const double *array, int size)
{
    if (size <= 0)
       return 0.0;

    double rv = array[0];
    for (int i = 1; i < size; i++)
       rv = qwtMin(rv, array[i]);

    return rv;
}


/*!
  \brief Find the largest value in an array
  \param array Pointer to an array
  \param size Array size
*/
double qwtGetMax(const double *array, int size)
{
    if (size <= 0)
       return 0.0;
    
    double rv = array[0];
    for (int i = 1; i < size; i++)
       rv = qwtMax(rv, array[i]);

    return rv;
}
