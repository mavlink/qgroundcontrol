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
 * File:    linklist.h
 * Purpose: header file for linked list API
 * Dependencies: 
 * Invokes:
 * Contents:
 * Structure definitions: 
 * Constant definitions: 
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __LINKLIST_H
#define __LINKLIST_H

#include "hdf.h"

/* Definitions for linked-list creation flags */
#define HUL_UNSORTED_LIST   0x0000
#define HUL_SORTED_LIST     0x0001

/* Type of the function to compare objects & keys */
typedef intn (*HULsearch_func_t)(const VOIDP obj, const VOIDP key);

/* Type of the function to compare two objects */
typedef intn (*HULfind_func_t)(const VOIDP obj1, const VOIDP obj2);

/* Linked list information structure used */
typedef struct node_info_struct_tag {
    VOIDP *obj_ptr;         /* pointer associated with the linked list node */
    struct node_info_struct_tag *next;   /* link to list node */
  }node_info_t;

/* Linked list head structure */
typedef struct list_head_struct_tag {
    uintn count;            /* # of nodes in the list */
    uintn flags;            /* list creation flags */
    HULfind_func_t cmp_func;    /* node comparison function */
    node_info_t *node_list; /* pointer to a linked list of nodes */
    node_info_t *curr_node; /* pointer to the current node when iterating */
  }list_head_t;

#if defined LIST_MASTER | defined LIST_TESTER

/* Define this in only one place */
#ifdef LIST_MASTER
/* Pointer to the list node free list */
static node_info_t *node_free_list=NULL;

#endif /* LIST_MASTER */

/* Useful routines for generally private use */

#endif /* LIST_MASTER | LIST_TESTER */

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/******************************************************************************
 NAME
     HULcreate_list - Create a linked list

 DESCRIPTION
    Creates a linked list.  The list may either be sorted or un-sorted, based
    on the comparison function.

 RETURNS
    Returns a pointer to the list if successful and NULL otherwise

*******************************************************************************/
list_head_t *HULcreate_list(HULfind_func_t find_func    /* IN: object comparison function */
);

/******************************************************************************
 NAME
     HULdestroy_list - Destroys a linked list

 DESCRIPTION
    Destroys a linked list created by HULcreate_list().  This function
    walks through the list and frees all the nodes, then frees the list head.
    Note: this function does not (currently) free the objects in the nodes,
    it just leaves 'em hanging.

 RETURNS
    Returns SUCCEED/FAIL.

*******************************************************************************/
intn HULdestroy_list(list_head_t *lst    /* IN: list to destroy */
);

/******************************************************************************
 NAME
     HULadd_node - Adds an object to a linked-list

 DESCRIPTION
    Adds an object to the linked list.  If the list is sorted, the comparison
    function is used to determine where to insert the node, otherwise it is
    inserted at the head of the list.

 RETURNS
    Returns SUCCEED/FAIL.

*******************************************************************************/
intn HULadd_node(list_head_t *lst,  /* IN: list to modify */
    VOIDP obj                       /* IN: object to add to the list */
);

/******************************************************************************
 NAME
     HULsearch_node - Search for an object in a linked-list

 DESCRIPTION
    Locate an object in a linked list using a key and comparison function.

 RETURNS
    Returns a pointer to the object found in the list, or NULL on failure.

*******************************************************************************/
VOIDP HULsearch_node(list_head_t *lst,  /* IN: list to search */
    HULsearch_func_t srch_func,       /* IN: function to use to find node */
    VOIDP key                       /* IN: key of object to search for */
);

/******************************************************************************
 NAME
     HULfirst_node - Get the first object in a linked-list

 DESCRIPTION
    Returns the first object in a linked-list and prepares the list for
    interating through.

 RETURNS
    Returns a pointer to the first object found in the list, or NULL on failure.

*******************************************************************************/
VOIDP HULfirst_node(list_head_t *lst   /* IN: list to search */
);

/******************************************************************************
 NAME
     HULnext_node - Get the next object in a linked-list

 DESCRIPTION
    Returns the next object in a linked-list by walking through the list

 RETURNS
    Returns a pointer to the next object found in the list, or NULL on failure.

*******************************************************************************/
VOIDP HULnext_node(list_head_t *lst   /* IN: list to search */
);

/******************************************************************************
 NAME
     HULremove_node - Removes an object from a linked-list

 DESCRIPTION
    Remove an object from a linked list.  The key and comparison function are
    provided locate the object to delete.

 RETURNS
    Returns a pointer to the object deleted from the list, or NULL on failure.

*******************************************************************************/
VOIDP HULremove_node(list_head_t *lst,  /* IN: list to modify */
    HULsearch_func_t srch_func,       /* IN: function to use to find node to remove */
    VOIDP key                       /* IN: object to add to the list */
);

/*--------------------------------------------------------------------------
 NAME
    HULshutdown
 PURPOSE
    Terminate various global items.
 USAGE
    intn HULshutdown()
 RETURNS
    Returns SUCCEED/FAIL
 DESCRIPTION
    Free various buffers allocated in the HUL routines.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Should only ever be called by the "atexit" function HDFend
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
intn 
HULshutdown(void);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

#endif /* __LINKLIST_H */

