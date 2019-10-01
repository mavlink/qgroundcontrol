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

/*------------------------------------------------------------------------------
 * File:    mfan.h
 * Author:  GeorgeV
 * Purpose: header file for the Multi-file Annotation Interface
 * Invokes: 
 * Contents:
 *          Structure definitions: ANnode, ANentry
 *          Constant definitions:  AN_DATA_LABEL, AN_DATA_DESC
 *          (-moved to hdf.h)      AN_FILE_LABEL, AN_FILE_DESC
 *
 *----------------------------------------------------------------------------*/

#ifndef _MFAN_H  /* avoid re-inclusion */
#define _MFAN_H

#include "H4api_adpt.h"

#include "hdf.h"

#if 0
/* enumerated types of the varous annotation types 
 * NOTE: moved to hdf.h since they are used by end users. */
typedef enum 
{ 
  AN_DATA_LABEL = 0, /* Data label */
  AN_DATA_DESC,      /* Data description */
  AN_FILE_LABEL,     /* File label */
  AN_FILE_DESC       /* File description */
} ann_type;
#endif

#if defined MFAN_MASTER | defined MFAN_TESTER
/* WE ARE IN MAIN ANNOTATION SOURCE FILE "mfan.c" */

/* PRIVATE variables and definitions */

/* This sturcture is used to find which file the annotation belongs to
 * and use the subsequent file specific annotation 'key' to find the 
 * annotation. The annotation atom group(ANIDGROUP) keeps track of 
 * all anotations across the file. */
typedef struct ANnode
{
  int32   file_id;  /* which file this annotation belongs to */
  int32   ann_key;  /* type/ref: used to find annotation in corresponding
                       TBBT in filerec_t->tree[]. */
  intn    new_ann;  /* flag */
} ANnode;

/*
 * This structure is an entry in the label/desc tree
 * for a label/desc in the file, it gives the ref of the label/desc,
 * and the tag/ref of the data item to which the label/desc relates 
 * The filerec_t->an_tree[] TBBT members will contain these entries.
 **/
typedef struct ANentry
{
  int32   ann_id;      /* annotation id */
  uint16  annref;      /* ref of annotation */
  uint16  elmtag;      /* tag of data */
  uint16  elmref;      /* ref of data */
} ANentry;


/* This is the size of the hash tables used for annotation IDs */
#define ANATOM_HASH_SIZE    64

/* Used to create unique 32bit keys from annotation type and reference number 
 *  This key is used to add nodes to a corresponding TBBT in 
 *  filrerec_t->an_tree[]. 
 *  ----------------------------
 *  | type(16bits) | ref(16bits) |
 *  -----------------------------*/
#define AN_CREATE_KEY(t,r) ((((int32)t & 0xffff) << 16) | r)

/* Obtain Reference number from key */
#define AN_KEY2REF(k)      ((uint16)((int32)k & 0xffff))

/* Obtain Annotation type from key */
#define AN_KEY2TYPE(k)     ((int32)((int32)k >> 16))

#else /* !defined MFAN_MASTER && !defined MFAN_TESTER */
/* WE are NOT in main ANNOTATION source file
 * Nothing EXPORTED except Public fcns */


/******************************************************************************
 NAME
   ANstart - open file for annotation handling

 DESCRIPTION
   Start annotation handling on the file return a annotation ID to the file.

 RETURNS
   A file ID or FAIL.
*******************************************************************************/
HDFLIBAPI int32 ANstart(int32 file_id /* IN: file to start annotation access on */);

/******************************************************************************
 NAME
   ANfileinfo - Report high-level information about the ANxxx interface for a given file.

 DESCRIPTION
   Reports general information about the number of file and object(i.e. data)
   annotations in the file. This routine is generally used to find
   the range of acceptable indices for ANselect calls.

 RETURNS
   Returns SUCCEED if successful and FAIL othewise

*******************************************************************************/
HDFLIBAPI intn  ANfileinfo(int32 an_id,         /* IN:  annotation interface id */
                        int32 *n_file_label, /* OUT: the # of file labels */
                        int32 *n_file_desc,  /* OUT: the # of file descriptions */
                        int32 *n_obj_label,  /* OUT: the # of object labels */ 
                        int32 *n_obj_desc    /* OUT: the # of object descriptions */);

/******************************************************************************
 NAME
   ANend - End annotation access to file file

 DESCRIPTION
   End annotation access to file.

 RETURNS
   SUCCEED if successful and  FAIL otherwise.
*******************************************************************************/
HDFLIBAPI int32 ANend(int32 an_id /* IN: Annotation ID of file to close */);

/******************************************************************************
 NAME
   ANcreate - create a new element annotation and return a handle(id)

 DESCRIPTION
   Creates a data annotation, returns an 'an_id' to work with the new 
   annotation which can either be a label or description.
   Valid annotation types are AN_DATA_LABEL for data labels and 
   AN_DATA_DESC for data descriptions.

 RETURNS
        An ID to an annotation which can either be a label or description.
*******************************************************************************/
HDFLIBAPI int32 ANcreate(int32 an_id,     /* IN: annotation interface ID */
                      uint16 elem_tag, /* IN: tag of item to be assigned annotation */
                      uint16 elem_ref, /* IN: reference number of itme to be assigned ann*/
                      ann_type type    /* IN: annotation type */);


/******************************************************************************
 NAME
	ANcreatef - create a new file annotation and return a handle(id)

 DESCRIPTION
    Creates a file annotation, returns an 'an_id' to work with the new 
    file annotation which can either be a label or description.
    Valid annotation types are AN_FILE_LABEL for file labels and
    AN_FILE_DESC for file descritpions.

 RETURNS
        An ID to an annotation which can either be a file label or description
*******************************************************************************/
HDFLIBAPI int32 ANcreatef(int32 an_id,  /* IN: annotation interface ID */
                       ann_type type /* IN:  annotation type */);

/******************************************************************************
 NAME
	ANselect - get an annotation ID from index of 'type'

 DESCRIPTION
    Get an annotation Id from index of 'type'.
    The position index is ZERO based

 RETURNS
    An ID to an annotation type which can either be a label or description 
*******************************************************************************/
HDFLIBAPI int32 ANselect(int32 an_id,  /* IN: annotation interface ID */
                      int32 index,  /* IN: index of annottion to get ID for */
                      ann_type type /* IN: annotation type */);

/******************************************************************************
 NAME
   ANnumann - find number of annotation of 'type' that  match the given element tag/ref 

 DESCRIPTION
   Find number of annotation of 'type' for the given element 
   tag/ref pair.Should not be used for File labels and
   descriptions.

 RETURNS
   number of annotation found if successful and FAIL (-1) otherwise

*******************************************************************************/
HDFLIBAPI intn  ANnumann(int32 an_id,     /* IN: annotation interface id */
                      ann_type type,   /* IN: annotation type */
                      uint16 elem_tag, /* IN: tag of item of which this is annotation */
                      uint16 elem_ref  /* IN: ref of item of which this is annotation*/);

/******************************************************************************
 NAME
   ANannlist - generate list of annotation ids of 'type' that match the given element tag/ref 

 DESCRIPTION
   Find and generate list of annotation ids of 'type' for the given 
   element tag/ref pair.Should not be used for File labels and
   descriptions.

 RETURNS
   number of annotations ids found if successful and FAIL (-1) otherwise

*******************************************************************************/
HDFLIBAPI intn  ANannlist(int32 an_id,     /* IN: annotation interface id */
                       ann_type type,   /* IN: annotation type */
                       uint16 elem_tag, /* IN: tag of item of which this is annotation */
                       uint16 elem_ref, /* IN: ref of item of which this is annotation*/
                       int32 ann_list[] /* OUT: array of ann_id's that match criteria.*/);

/******************************************************************************
 NAME
   ANannlen - get length of annotation givne annotation id

 DESCRIPTION
   Uses the annotation id to find ann_key & file_id

 RETURNS
   length of annotation if successful and FAIL (-1) otherwise

*******************************************************************************/
HDFLIBAPI int32 ANannlen(int32 ann_id /* IN: annotation id */);

/******************************************************************************
 NAME
   ANwriteann - write annotation given ann_id

 DESCRIPTION
   Checks for pre-existence of given annotation, replacing old one if it
   exists. Writes out annotation.

 RETURNS
   SUCCEED (0) if successful and FAIL (-1) otherwise

*******************************************************************************/
HDFLIBAPI int32 ANwriteann(int32 ann_id,    /* IN: annotation id */
                        const char *ann, /* IN: annotation to write */
                        int32 annlen     /* IN: length of annotation*/);

/******************************************************************************
 NAME
   ANreadann - read annotation given ann_id

 DESCRIPTION
   Gets tag and ref of annotation.  Finds DD for that annotation.
   Reads the annotation, taking care of NULL terminator, if necessary.

 RETURNS
   SUCCEED (0) if successful and FAIL (-1) otherwise

*******************************************************************************/
HDFLIBAPI int32 ANreadann(int32 ann_id, /* IN: annotation id (handle) */
                       char *ann,    /* OUT: space to return annotation in */
                       int32 maxlen  /* IN: size of space to return annotation in */);

/******************************************************************************
 NAME
	ANendaccess - end access to an annotation given it's id

 DESCRIPTION
    Terminates access to an annotation. For now does nothing

 RETURNS
    SUCCEED(0) or FAIL(-1)
*******************************************************************************/
HDFLIBAPI intn  ANendaccess(int32 ann_id /* IN: annotation id */);

/******************************************************************************
 NAME
   ANget_tagref - get tag/ref pair for annotation based on type and index

 DESCRIPTION
   Get the tag/ref of the annotation based on  the type and index of the 
   annotation. The position index is zero based

 RETURNS
   A tag/ref pairt to an annotation type which can either be a 
   label or description.

*******************************************************************************/
HDFLIBAPI int32 ANget_tagref(int32 an_id,    /* IN: annotation interface ID */
                          int32 index,    /* IN: index of annotation to get tag/ref for*/
                          ann_type type,  /* IN: annotation type */
                          uint16 *ann_tag,/* OUT: Tag for annotation */ 
                          uint16 *ann_ref /* OUT: ref for annotation */);

/******************************************************************************
 NAME
   ANid2tagref -- get tag/ref given annotation id

 DESCRIPTION
   Uses the annotation id to find ann_node entry which contains ann_ref

 RETURNS
   SUCCEED(0) if successful and FAIL (-1) otherwise. 
*******************************************************************************/
HDFLIBAPI int32 ANid2tagref(int32 ann_id,    /* IN: annotation id */
                         uint16 *ann_tag, /* OUT: Tag for annotation */
                         uint16 *ann_ref  /* OUT: ref for annotation */);

/******************************************************************************
 NAME
   ANtagref2id -- get annotation id given tag/ref

 DESCRIPTION
   Gets the annotation id of the annotation given the tag/ref of
   the annotation itself and the annotation interface id.

 RETURNS
   Annotation id of annotation if successful and FAIL(-1) otherwise. 
*******************************************************************************/
HDFLIBAPI int32 ANtagref2id(int32 an_id,    /* IN  Annotation interface id */
                         uint16 ann_tag, /* IN: Tag for annotation */
                         uint16 ann_ref  /* IN: ref for annotation */);

/******************************************************************************
 NAME
   ANatype2tag - annotation type to corresponding annotation TAG

 DESCRIPTION
   Translate annotation type to corresponding TAG.

 RETURNS
   Returns TAG corresponding to annotatin type.
*******************************************************************************/
HDFLIBAPI uint16 ANatype2tag(ann_type atype /* IN: Annotation type */);

/******************************************************************************
 NAME
   ANtag2atype - annotation TAG to corresponding annotation type

 DESCRIPTION
   Translate annotation TAG to corresponding atype

 RETURNS
   Returns type corresponding to annotatin TAG.
*******************************************************************************/
HDFLIBAPI ann_type ANtag2atype(uint16 atag /* IN: annotation tag */);


#endif /* !defined MFAN_MASTER && !MFAN_TESTER */

#endif /* _MFAN_H */
