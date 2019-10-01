/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF/releases/.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id$ */

/*-----------------------------------------------------------------------------
 * File:    dynarray.h
 * Purpose: header file for dynamic array API
 * Dependencies: 
 * Invokes:
 * Contents:
 * Structure definitions: 
 * Constant definitions: 
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __DYNARRAY_H
#define __DYNARRAY_H

#include "hdf.h"

/*
    Define the pointer to the dynarray without giving outside routines access
    to the internal workings of the structure.
*/
typedef struct dynarray_tag *dynarr_p;

#if defined DYNARRAY_MASTER | defined DYNARRAY_TESTER
typedef struct dynarray_tag 
  {
      intn num_elems;       /* Number of elements in the array currently */
      intn incr_mult;       /* Multiple to increment the array size by */
      VOIDP *arr;           /* Pointer to the actual array of void *'s */
  }dynarr_t;

#endif /* DYNARRAY_MASTER | DYNARRAY_TESTER */

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/******************************************************************************
 NAME
     DAcreate_array - Create a dynarray

 DESCRIPTION
    Create a dynarray for later use.  This routine allocates the dynarray
    structure and creates a dynarray with the specified minimum size.

 RETURNS
    Returns pointer to the dynarray created if successful and NULL otherwise

*******************************************************************************/
dynarr_p DAcreate_array(intn start_size,      /* IN: Initial array size */
    intn incr_mult                  /* IN: multiple to create additional elements in */
);

/******************************************************************************
 NAME
     DAdestroy_array - Destroy a dynarray

 DESCRIPTION
    Destroy an existing dynarray from use.  This routine de-allocates the
    dynarray structure and deletes the current dynarray.

 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn DAdestroy_array(dynarr_p arr,  /* IN: Array to destroy */
        intn free_elem              /* IN: whether to free each element */
);

/******************************************************************************
 NAME
     DAdestroy_array - Get the current size of a dynarray

 DESCRIPTION
    Get the number of elements in use currently.

 RETURNS
    Returns # of dynarray elements if successful and FAIL otherwise

*******************************************************************************/
intn DAsize_array(dynarr_p arr   /* IN: Array to get size of */
);

/******************************************************************************
 NAME
     DAget_elem - Get an element from a dynarray

 DESCRIPTION
    Retrieve an element from a dynarray.  If the element to be retrieved is
    beyond the end of the currently allocated array elements, the array is
    not extended, a NULL pointer is merely returned.

 RETURNS
    Returns object ptr if successful and NULL otherwise

*******************************************************************************/
VOIDP DAget_elem(dynarr_p arr_ptr, /* IN: Array to access */
    intn elem                       /* IN: Array element to retrieve */
);

/******************************************************************************
 NAME
     DAset_elem - Set an element pointer for a dynarray

 DESCRIPTION
    Set an element pointer for a dynarray.  If the element to be set is
    beyond the end of the currently allocated array elements, the array is
    extended by whatever multiple of the incr_mult is needed to expand the
    # of array elements to include the array element to set.

 RETURNS
    Returns SUCCEED if successful and NULL otherwise

*******************************************************************************/
intn DAset_elem(dynarr_p arr_ptr,  /* IN: Array to access */
    intn elem,                      /* IN: Array element to set */
    VOIDP obj                       /* IN: Pointer to the object to store */
);

/*****************************************************************************
 NAME
     DAdel_elem - Delete an element from a dynarray

 DESCRIPTION
    Retrieve an element from a dynarray & delete it from the dynarray.  If the
    element to be retrieved is beyond the end of the currently allocated array
    elements, the array is not extended, a NULL pointer is merely returned.

 RETURNS
    Returns object ptr if successful and NULL otherwise

*******************************************************************************/
VOIDP DAdel_elem(dynarr_p arr_ptr, /* IN: Array to access */
    intn elem                       /* IN: Array element to retrieve */
);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

#endif /* __DYNARRAY_H */

