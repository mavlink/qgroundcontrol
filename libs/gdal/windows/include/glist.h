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

/************************************************************************
  Credits:
         Original code is part of the public domain 'Generic List Library'
         by Keith Pomakis(kppomaki@jeeves.uwaterloo.ca)-Spring, 1994
         It has been modifed to adhere to HDF coding standards.

  1996/05/29 - George V.
 ************************************************************************/

/* $Id$ */

#ifndef GLIST_H
#define GLIST_H

#include "hdf.h" /* needed for data types */

/* Structure for each element in the list */
typedef struct GLE_struct {
    VOIDP                pointer;   /* data element itself */
    struct GLE_struct   *previous; /* previous element */
    struct GLE_struct   *next;     /* next element */
} Generic_list_element;

/* List info structure */
typedef struct GLI_struct {
    Generic_list_element   *current;               /* current element */
    Generic_list_element   pre_element;            /* pre element */
    Generic_list_element   post_element;           /* post element */
    Generic_list_element   deleted_element;        /* deleted element */
    intn                  (*lt)(VOIDP a, VOIDP b); /* sort fcn */
    uint32                 num_of_elements;        /* number of elements */
} Generic_list_info;

/* Top most List structure, handle to the list */
typedef struct GL_struct {
    Generic_list_info *info;
} Generic_list;

/* Define a Stack and Queue */
#define Generic_stack Generic_list
#define Generic_queue Generic_list

/* Function declarations 
   Descriptions for the General List routines can be found in 'glist.c'
   while the stack and queue routines are found below
 */

/******************************************************************************
 NAME
     HDGLinitialize_list
 DESCRIPTION
     Every list must be initialized before it is used.  The only time it is
     valid to re-initialize a list is after it has been destroyed.
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
intn HDGLinitialize_list(Generic_list *list /* IN: list */);

/******************************************************************************
 NAME
     HDGLinitialize_sorted_list
 DESCRIPTION
    This function initializes a sorted list.  A less-than function must be
    specified which accepts two pointers, a and b, and returns TRUE
    (non-zero) if a is less than b, FALSE otherwise.

    Once a list is initialized in this way, all of the generic list
    functions described above can be used, except for:

        void HDGLadd_to_beginning(Generic_list list, void *pointer);
        void HDGLadd_to_end(Generic_list list, void *pointer);
        void *HDGLremove_from_beginning(Generic_list list);
        void *HDGLremove_from_end(Generic_list list);

    and the list will remain sorted by the criteria specified by the
    less-than function.  The only time it is valid to re-initialize a list
    is after it has been destroyed.
 RETURNS
     SUCEED/FAIL
*******************************************************************************/
intn HDGLinitialize_sorted_list(Generic_list *list/*IN: list */, 
                                intn (*lt)(VOIDP a, VOIDP b)/*IN:sort fcn */);

/******************************************************************************
 NAME
     destory_list
 DESCRIPTION
    When a list is no longer needed, it should be destroyed.  This process
    will automatically remove all remaining objects from the list.  However,
    the memory for these objects will not be reclaimed, so if the objects
    have no other references, care should be taken to purge the list and
    free all objects before destroying the list.

    It is an error to destroy a list more than once (unless it has been
    re-initialized in the meantime).
 RETURNS
     Nothing
*******************************************************************************/
void HDGLdestroy_list(Generic_list *list /*IN: list */);

/******************************************************************************
 NAME
    HDGLadd_to_beginning
 DESCRIPTION
    This function will add the specified object to the beginning of the
    list.  The pointer must not be NULL.
 RETURNS
    SUCCEED/FAIL
*******************************************************************************/
intn HDGLadd_to_beginning(Generic_list list, /*IN: list */
                          VOIDP pointer /*IN: data element */ );

/******************************************************************************
 NAME
     HDGLadd_to_end
 DESCRIPTION
    This function will add the specified object to the end of the
    list.  The pointer must not be NULL.
 RETURNS
    SUCCEED/FAIL
*******************************************************************************/
intn HDGLadd_to_end(Generic_list list, /*IN: list */
                    VOIDP pointer /*IN: data element */);

/******************************************************************************
 NAME
     HDGLadd_to_list
 DESCRIPTION
    This function will add the specified object to the list.  The pointer
    must not be NULL.
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
intn HDGLadd_to_list(Generic_list list, /*IN: list */
                     VOIDP pointer /*IN: data element */);

/******************************************************************************
 NAME
     HDGLremove_from_beginning
 DESCRIPTION
    This function will remove the first object from the beginning of the
    list and return it.  If the list is empty, NULL is returned.
 RETURNS
    First Element if successful and NULL otherwise.
*******************************************************************************/
VOIDP HDGLremove_from_beginning(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLremove_from_end
 DESCRIPTION
    This function will remove the last object from the end of the list and
    return it.  If the list is empty, NULL is returned.
 RETURNS
    Last element if successfull and NULL otherwise
*******************************************************************************/
VOIDP HDGLremove_from_end(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLremove_from_list
 DESCRIPTION
    This function will remove the specified object from the list and return
    it.  If the specified object does not exist in the list, NULL is
    returned.  If the specified object exists in the list more than once,
    only the last reference to it is removed.

 RETURNS
    Element removed if successful and NULL otherwise
*******************************************************************************/
VOIDP HDGLremove_from_list(Generic_list list, /*IN: list */
                           VOIDP pointer /*IN: data element */);

/******************************************************************************
 NAME
     HDGLremove_current
 DESCRIPTION
    This function will remove the current object from the list and return
    it.  If the current object has already been removed, if current points
    to the beginning or end of the list, or if the list is empty, NULL is
    returned.
 RETURNS
    Current element if successful and NULL otherwise.
*******************************************************************************/
VOIDP HDGLremove_current(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLremove_all
 DESCRIPTION
    This function will remove all objects from the list.  Note that the
    memory for these objects will not be reclaimed, so if the objects have
    no other references, it is best to avoid this function and remove the
    objects one by one, freeing them when necessary.
 RETURNS
    Nothing
*******************************************************************************/
void HDGLremove_all(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLpeek_at_beginning
 DESCRIPTION
    This function will return the first object in the list.  If the list is
    empty, NULL is returned.
 RETURNS
    First element in list if non-empty, otherwise NULL.
*******************************************************************************/
VOIDP HDGLpeek_at_beginning(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLpeek_at_end
 DESCRIPTION
    This function will return the last object in the list.  If the list is
    empty, NULL is returned.
 RETURNS
    Last element in list if non-empty, otherwise NULL.
*******************************************************************************/
VOIDP HDGLpeek_at_end(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLfirst_in_list
 DESCRIPTION
    This function will return the first object in the list and mark it as
    the current object.  If the list is empty, NULL is returned.
 RETURNS
    First element in list if non-empty, otherwise NULL.
*******************************************************************************/
VOIDP HDGLfirst_in_list(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLcurrent_in_list
 DESCRIPTION
    This function will return the object in the list that is considered
    the current object (as defined by the surrounding functions).  If the
    current object has just been removed, if current points to the
    beginning or end of the list, or if the list is empty, NULL is
    returned.
 RETURNS
    Current element in list if non-empty, otherwise NULL.
*******************************************************************************/
VOIDP HDGLcurrent_in_list(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLlast_in_list
 DESCRIPTION
    This function will return the last object in the list and mark it as
    the current object.  If the list is empty, NULL is returned.
 RETURNS
    Last element in list if non-empty, otherwise NULL.
*******************************************************************************/
VOIDP HDGLlast_in_list(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLnext_in_list
 DESCRIPTION
    This function will return the next object in the list and mark it as
    the current object.  If the end of the list is reached, or if the list
    is empty, NULL is returned.
 RETURNS
    Next element in list if non-empty, otherwise NULL.
*******************************************************************************/
VOIDP HDGLnext_in_list(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLprevious_in_list
 DESCRIPTION
    This function will return the previous object in the list and mark it
    as the current object.  If the beginning of the list is reached, or if
    the list is empty, NULL is returned.
 RETURNS
    Previous element in list if non-empty, otherwise NULL.
*******************************************************************************/
VOIDP HDGLprevious_in_list(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLreset_to_beginning
 DESCRIPTION
    This function will reset the list to the beginning.  Therefore, current
    points to the beginning of the list, and the next object in the list is
    the first object.
 RETURNS
     Nothing
*******************************************************************************/
void HDGLreset_to_beginning(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLreset_to_end
 DESCRIPTION
    This function will reset the list to the end.  Therefore, current
    points to the end of the list, and the previous object in the list is
    the last object.
 RETURNS
     Nothing
*******************************************************************************/
void HDGLreset_to_end(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLnum_of_objects
 DESCRIPTION
    This function will determine the number of objects in the list.
 RETURNS
    Number of objects in list
*******************************************************************************/
intn HDGLnum_of_objects(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLis_empty
 DESCRIPTION
    Finds if list is empty
 RETURNS
    This function will return TRUE (1) if the list is empty, and FALSE (0)
    otherwise.
*******************************************************************************/
intn HDGLis_empty(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLis_in_list
 DESCRIPTION
     Detemines if the object is in the list.
 RETURNS
    This function will return TRUE (1) if the specified object is a member
    of the list, and FALSE (0) otherwise.
*******************************************************************************/
intn HDGLis_in_list(Generic_list list, /*IN: list */
               VOIDP pointer /*IN: data element */);

/******************************************************************************
 NAME
     HDGLcopy_list
 DESCRIPTION
    This function will make a copy of the list.  The objects themselves
    are not copied; only new references to them are made.  The new list
    loses its concept of the current object.
 RETURNS
    A copy of the orginal list.
*******************************************************************************/
Generic_list HDGLcopy_list(Generic_list list /*IN: list */);

/******************************************************************************
 NAME
     HDGLperform_on_list
 DESCRIPTION
    This function will perform the specified function on each object in the
    list.  Any optional arguments required can be passed through args.
 RETURNS
    Nothing
*******************************************************************************/
void HDGLperform_on_list(Generic_list list, /*IN: list */
                         void (*fn)(VOIDP pointer, VOIDP args), /* IN: iterator fcn */
                         VOIDP args /*IN: args to iterator fcn */);

/******************************************************************************
 NAME
     HDGLfirst_that
 DESCRIPTION
     This function will find and return the first object in the list which
     causes the specified function to return a TRUE (non-zero) value.  Any
     optional arguments required can be passed through args.  The found
     object is then marked as the current object.  If no objects in the list
     meet the criteria of the specified function, NULL is returned.
 RETURNS
     Element if successful and NULL otherwise.
*******************************************************************************/
VOIDP HDGLfirst_that(Generic_list list, /*IN: list */
                     intn (*fn)(VOIDP pointer, VOIDP args), /* IN: iterator fcn */
                     VOIDP args /*IN: args to iterator fcn */);

/******************************************************************************
 NAME
     HDGLnext_that
 DESCRIPTION
     This function will find and return the next object in the list which
     causes the specified function to return a TRUE (non-zero) value.  Any
     optional arguments required can be passed through args.  The found
     object is then marked as the current object.  If there are no objects
     left in the list that meet the criteria of the specified function,
     NULL is returned.
 RETURNS
     Element if successful and NULL otherwise.
*******************************************************************************/
VOIDP HDGLnext_that(Generic_list list, /*IN: list */
                    intn (*fn)(VOIDP pointer, VOIDP args), /* IN: iterator fcn */
                    VOIDP args /*IN: args to iterator fcn */);

/******************************************************************************
 NAME
     HDGLprevious_that
 DESCRIPTION
     This function will find and return the previous object in the list
     which causes the specified function to return a TRUE (non-zero) value.
     Any optional arguments required can be passed through args.  The found
     object is then marked as the current object.  If there are no objects
     left in the list that meet the criteria of the specified function,
     NULL is returned.
 RETURNS
     Element if successful and NULL otherwise.
*******************************************************************************/
VOIDP HDGLprevious_that(Generic_list list, /*IN: list */
                        intn (*fn)(VOIDP pointer, VOIDP args), /* IN: iterator fcn */
                        VOIDP args /*IN: args to iterator fcn */);

/******************************************************************************
 NAME
     HDGLlast_that
 DESCRIPTION
     This function will find and return the last object in the list which
     causes the specified function to return a TRUE (non-zero) value.  Any
     optional arguments required can be passed through args.  The found
     object is then marked as the current object.  If no objects in the
     list meet the criteria of the specified function, NULL is returned.
 RETURNS
     Element if successful and NULL otherwise.
*******************************************************************************/
VOIDP HDGLlast_that(Generic_list list, /*IN: list */
                    intn (*fn)(VOIDP pointer, VOIDP args), /* IN: iterator fcn */
                    VOIDP args /*IN: args to iterator fcn */);

/******************************************************************************
 NAME
    HDGLall_such_that
 DESCRIPTION
    This function will return a new list containing all of the objects in
    the specified list which cause the specified function to return a TRUE
    (non-zero) value.  Any optional arguments required can be passed
    through args. The objects themselves are not copied; only new
    references to them are made.
 RETURNS
    New list if successful and empty if not.
*******************************************************************************/
Generic_list HDGLall_such_that(Generic_list list, /*IN: list */
                               intn (*fn)(VOIDP pointer, VOIDP args), /* IN: iterator fcn */
                               VOIDP args /*IN: args to iterator fcn */);

/******************************************************************************
 NAME
     HDGLremove_HDGLall_such_that
 DESCRIPTION
    This function will remove all objects in the list which cause the
    specified function to return a TRUE (non-zero) value.  Any optional
    arguments required can be passed through args.  Note that the memory
    for these objects will not be reclaimed, so if the objects have
    no other references, it is best to avoid this function and remove the
    objects one by one, freeing them when necessary.
 RETURNS
     Nothing
*******************************************************************************/
void HDGLremove_all_such_that(Generic_list list, /*IN: list */
                              intn (*fn)(VOIDP pointer, VOIDP args), /* IN: iterator fcn */
                              VOIDP args /*IN: args to iterator fcn */);


/****************************************************************************/
/* 
 * Stack operations 
 */

/******************************************************************************
 NAME
     HDGSinitialize_stack
 DESCRIPTION
    Every stack must be initialized before it is used.  The only time it is
    valid to re-initialize a stack is after it has been destroyed.
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
#define HDGSinitialize_stack HDGLinitialize_list

/******************************************************************************
 NAME
     HDGSdestroy_stack     
 DESCRIPTION
    When a stack is no longer needed, it should be destroyed.  This process
    will automatically remove all remaining objects from the stack.
    However, the memory for these objects will not be reclaimed, so if the
    objects have no other references, care should be taken to purge the
    stack and free all objects before destroying the stack.

    It is an error to destroy a stack more than once (unless it has been
    re-initialized in the meantime).
 RETURNS
     Nothing
*******************************************************************************/
#define HDGSdestroy_stack    HDGLdestroy_list

/******************************************************************************
 NAME
     HDGSpush
 DESCRIPTION
    This function will HDGSpush the specified object onto the stack.  The
    pointer must not be NULL.
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
#define HDGSpush             HDGLadd_to_beginning

/******************************************************************************
 NAME
     HDGSpop
 DESCRIPTION
    This function will HDGSpop the first object from the top of the stack and
    return it.  If the stack is empty, NULL is returned.
 RETURNS
     First element of the top of the stack
*******************************************************************************/
#define HDGSpop              HDGLremove_from_beginning

/******************************************************************************
 NAME
     HDGSpop_all
 DESCRIPTION
    This function will HDGSpop all objects from the stack.  Note that the
    memory for these objects will not be reclaimed, so if the objects have
    no other references, it is best to avoid this function and HDGSpop the
    objects one by one, freeing them when necessary.
 RETURNS
     Nothing
*******************************************************************************/
#define HDGSpop_all          HDGLremove_all

/******************************************************************************
 NAME
     HDGSpeek_at_top
 DESCRIPTION
    This function will return the object on the top of the stack.  If the
    stack is empty, NULL is returned.
 RETURNS
     Element at top of stack.
*******************************************************************************/
#define HDGSpeek_at_top      HDGLpeek_at_beginning

/******************************************************************************
 NAME
     HDGScopy_stack
 DESCRIPTION
    This function will return a copy of the stack.  The objects themselves
    are not copied; only new references to them are made.
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
#define HDGScopy_stack       HDGLcopy_list


/****************************************************************************/
/* 
 * Queue operations 
 */

/******************************************************************************
 NAME
     HDGQinitialize_queue
 DESCRIPTION
    Every queue must be initialized before it is used.  The only time it is
    valid to re-initialize a queue is after it has been destroyed.
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
#define HDGQinitialize_queue HDGLinitialize_list

/******************************************************************************
 NAME
     HDGQdestroy_queue
 DESCRIPTION
    When a queue is no longer needed, it should be destroyed.  This process
    will automatically remove all remaining objects from the queue.
    However, the memory for these objects will not be reclaimed, so if the
    objects have no other references, care should be taken to purge the
    queue and free all objects before destroying the queue.

    It is an error to destroy a queue more than once (unless it has been
    re-initialized in the meantime).
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
#define HDGQdestroy_queue    HDGLdestroy_list

/******************************************************************************
 NAME
     HDGQenqueue     
 DESCRIPTION
    This function will add the specified object to the tail of the queue.
    The pointer must not be NULL.
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
#define HDGQenqueue          HDGLadd_to_end

/******************************************************************************
 NAME
     HDGQdequeue
 DESCRIPTION
    This function will remove the first object from the head of the queue
    and return it.  If the queue is empty, NULL is returned.
 RETURNS
    First element in the queue in non-empty, else NULL.
*******************************************************************************/
#define HDGQdequeue          HDGLremove_from_beginning

/******************************************************************************
 NAME
     HDGQdequeue_all
 DESCRIPTION
    This function will remove all objects from the queue.  Note that the
    memory for these objects will not be reclaimed, so if the objects have
    no other references, it is best to avoid this function and HDGQdequeue the
    objects one by one, freeing them when necessary.
 RETURNS
     Nothing
*******************************************************************************/
#define HDGQdequeue_all      HDGLremove_all

/******************************************************************************
 NAME
     HDGQpeek_at_head
 DESCRIPTION
    This function will return the object at the head of the queue.  If the
    queue is empty, NULL is returned.
 RETURNS
    First element in the queue in non-empty, else NULL.
*******************************************************************************/
#define HDGQpeek_at_head     HDGLpeek_at_beginning

/******************************************************************************
 NAME
     HDGQpeek_at_tail
 DESCRIPTION
    This function will return the object at the tail of the queue.  If the
    queue is empty, NULL is returned.
 RETURNS
    Last element in the queue in non-empty, else NULL.
*******************************************************************************/
#define HDGQpeek_at_tail     HDGLpeek_at_end

/******************************************************************************
 NAME
     HDGQcopy_queue
 DESCRIPTION
    This function will return a copy of the queue.  The objects themselves
    are not copied; only new references to them are made.
 RETURNS
     SUCCEED/FAIL
*******************************************************************************/
#define HDGQcopy_queue       HDGLcopy_list

#endif /* GLIST_H */

