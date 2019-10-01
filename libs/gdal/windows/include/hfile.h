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

/*+ hfile.h
   *** Header for hfile.c, routines for low level data element I/O
   + */

#ifndef HFILE_H
#define HFILE_H

#include "H4api_adpt.h"

#include "tbbt.h"
#include "bitvect.h"
#include "atom.h"
#include "linklist.h"
#include "dynarray.h"

/* Magic cookie for HDF data files */
#define MAGICLEN 4  /* length */
#define HDFMAGIC "\016\003\023\001"     /* ^N^C^S^A */

/* sizes of elements in a file.  This is necessary because
   the size of variables need not be the same as in the file
   (cannot use sizeof) */
#define DD_SZ 12    /* 2+2+4+4 */
#define NDDS_SZ 2
#define OFFSET_SZ 4

/* invalid offset & length to indicate a partially defined element 
* written to the HDF file i.e. can handle the case where the the
* element is defined but not written out */
#define INVALID_OFFSET -1
#define INVALID_LENGTH -1


/* ----------------------------- Version Tags ----------------------------- */
/* Library version numbers */

#define LIBVER_MAJOR    4
#define LIBVER_MINOR    2 
#define LIBVER_RELEASE  14 
#define LIBVER_SUBRELEASE ""   /* For pre-releases like snap0       */
                                /* Empty string for real releases.           */
#define LIBVER_STRING   "HDF Version 4.2 Release 14, June 26, 2018"
#define LIBVSTR_LEN    80   /* length of version string  */
#define LIBVER_LEN  92      /* 4+4+4+80 = 92 */
/* end of version tags */

/* -------------------------- File I/O Functions -------------------------- */
/* FILELIB -- file library to use for file access: 1 stdio, 2 fcntl
   default to stdio library i.e. UNIX buffered I/O */

#ifndef FILELIB
#   define FILELIB UNIXBUFIO    /* UNIX buffered I/O is the default */
#endif /* FILELIB */

#if (FILELIB == UNIXBUFIO)
/* using C buffered file I/O routines to access files */
#include <stdio.h>
typedef FILE *hdf_file_t;
#if defined SUN && defined (__GNUC__)
#   define HI_OPEN(p, a)       (((a) & DFACC_WRITE) ? \
                                fopen((p), "r+") : fopen((p), "r"))
#   define HI_CREATE(p)        (fopen((p), "w+"))
#else /* !SUN w/ GNU CC */
#   define HI_OPEN(p, a)       (((a) & DFACC_WRITE) ? \
                                fopen((p), "rb+") : fopen((p), "rb"))
#   define HI_CREATE(p)        (fopen((p), "wb+"))
#endif /* !SUN w/ GNU CC */
#   define HI_READ(f, b, n)    (((size_t)(n) == (size_t)fread((b), 1, (size_t)(n), (f))) ? \
                                SUCCEED : FAIL)
#   define HI_WRITE(f, b, n)   (((size_t)(n) == (size_t)fwrite((b), 1, (size_t)(n), (f))) ? \
                                SUCCEED : FAIL)
#   define HI_CLOSE(f)   (((f = ((fclose(f)==0) ? NULL : f))==NULL) ? SUCCEED:FAIL)
#   define HI_FLUSH(f)   (fflush(f)==0 ? SUCCEED : FAIL)
#   define HI_SEEK(f,o)  (fseek((f), (long)(o), SEEK_SET)==0 ? SUCCEED : FAIL)
#   define HI_SEEK_CUR(f,o)  (fseek((f), (long)(o), SEEK_CUR)==0 ? SUCCEED : FAIL)
#   define HI_SEEKEND(f) (fseek((f), (long)0, SEEK_END)==0 ? SUCCEED : FAIL)
#   define HI_TELL(f)    (ftell(f))
#   define OPENERR(f)    ((f) == (FILE *)NULL)
#endif /* FILELIB == UNIXBUFIO */

#if (FILELIB == UNIXUNBUFIO)
/* using UNIX unbuffered file I/O routines to access files */
typedef int hdf_file_t;
#   define HI_OPEN(p, a)       (((a) & DFACC_WRITE) ? \
                                    open((p), O_RDWR) : open((p), O_RDONLY))
#   define HI_CREATE(p)         (open((p), O_RDWR | O_CREAT | O_TRUNC, 0666))
#   define HI_CLOSE(f)          (((f = ((close(f)==0) ? NULL : f))==NULL) ? SUCCEED:FAIL)
#   define HI_FLUSH(f)          (SUCCEED)
#   define HI_READ(f, b, n)     (((n)==read((f), (char *)(b), (n))) ? SUCCEED : FAIL)
#   define HI_WRITE(f, b, n)    (((n)==write((f), (char *)(b), (n))) ? SUCCEED : FAIL)
#   define HI_SEEK(f, o)        (lseek((f), (off_t)(o), SEEK_SET)!=(-1) ? SUCCEED : FAIL)
#   define HI_SEEKEND(f)        (lseek((f), (off_t)0, SEEK_END)!=(-1) ? SUCCEED : FAIL)
#   define HI_TELL(f)           (lseek((f), (off_t)0, SEEK_CUR))
#   define OPENERR(f)           (f < 0)
#endif /* FILELIB == UNIXUNBUFIO */

#if (FILELIB == MACIO)
/* using special routines to redirect to Mac Toolkit I/O */
typedef short hdf_file_t;
#   define HI_OPEN(x,y)         mopen(x,y)
#   define HI_CREATE(name)      mopen(name, DFACC_CREATE)
#   define HI_CLOSE(x)          (((x = ((mclose(x)==0) ? NULL : x))==NULL) ? SUCCEED:FAIL)
#   define HI_FLUSH(a)          (SUCCEED)
#   define HI_READ(a,b,c)       mread(a, (char *) b, (int32) c)
#   define HI_WRITE(a,b,c)      mwrite(a, (char *) b, (int32) c)
#   define HI_SEEK(x,y)         mlseek(x, (int32 )y, 0)
#   define HI_SEEKEND(x)        mlseek(x, 0L, 2)
#   define HI_TELL(x)           mlseek(x,0L,1)
#   define DF_OPENERR(f)        ((f) == -1)
#   define OPENERR(f)           (f < 0)
#endif /* FILELIB == MACIO */


/* ----------------------- Internal Data Structures ----------------------- */
/* The internal structure used to keep track of the files opened: an
   array of filerec_t structures, each has a linked list of ddblock_t.
   Each ddblock_t struct points to an array of dd_t structs. 

   File Header(4 bytes)
   ===================
   <--- 32 bits ----->
   ------------------
   |HDF magic number |
   ------------------

   HDF magic number - 0x0e031301 (Hexadecimal)

   Data Descriptor(DD - 12 bytes)
   ==============================
   <-  16 bits -> <- 16 bits -> <- 32 bits -> <- 32 bits ->
   --------------------------------------------------------
   |    Tag      | reference   |  Offset     |  Length    |
   |             | number      |             |            |
   --------------------------------------------------------
        \____________/
               V
        tag/ref (unique data indentifier in file)
   
   Tag  -- identifies the type of data, 16 bit unsigned integer whose
           value ranges from 1 - 65535. Tags are assigned by NCSA.
           The HDF tag space is divided as follows based on the 2 highest bits:

              00: NCSA reserved ordinary tags
              01: NCSA reserved special tags(i.e. regular tags made into 
                                                  linked-block, external, 
                                                  compressed or chunked.)
              10, 11: User tags.

           Current tag assingments are:
           00001 - 32767  - reserved for NCSA use
                            00001 - 16383 - NCSA regular tags
                            16384 - 32767 - NCSA special tags
           32768 - 64999  - user definable
           65000 - 65535  - reserved for expansion of format

   Refererence number - 16 bit unsigned integer whose assignment is not
          gauranteed to be consecutive. Provides a way to distinguish 
          elements with the same tag in the file.

   Offset - Specifies the byte offset of the data element from the 
            beginning of the file - 32 bit unsigned integer

   Length - Indicates the byte length of the data element
            32 bit unsigned integer

   Data Descriptor Header(DDH - 6 bytes)
   ====================================
   <-  16 bits -> <- 32 bits ->
   -----------------------------
   | Block Size  | Next Block  |
   -----------------------------

   Block Size - Indicates the number of DD's in the DD Block,
                16 bit unsigned integer value
   Next Block - Gives the offset of the next DD Block. The last DD Block has
                a ZERO(0) in the "Next Block" field in the DDH.
                32 bit unsigned integer value

   Data Descriptor Block(DDB - variable size)
   ==========================================
   <- DD Header -> <--------------- DD's --------------------->
   --------------------------------------------------------...-
   |Block | Next  | DD | DD | DD | DD | DD | DD | DD | DD |...|
   |Size  | Block |    |    |    |    |    |    |    |    |   |
   --------------------------------------------------------...-
   <-------------------------- DD Block ---------------------->

   Note: default number of DD's in a DD Block is 16

   HDF file layout
   =============================
   (one example)
   ---------------------------------------------------------------------
   | FH | DD Block | Data | DD Block | Data | DD Block | Data | .....
   ---------------------------------------------------------------------
 
*/

/* record of each data decsriptor */
typedef struct dd_t
  {
      uint16      tag;          /* Tag number of element i.e. type of data */
      uint16      ref;          /* Reference number of element */
      int32       length;       /* length of data element */
      int32       offset;       /* byte offset of data element from */
      struct ddblock_t *blk;    /* Pointer to the block this dd is in */
  }                             /* beginning of file */
dd_t;

/* version tags */
typedef struct version_t
  {
      uint32      majorv;       /* major version number */
      uint32      minorv;       /* minor version number */
      uint32      release;      /* release number */
      char        string[LIBVSTR_LEN + 1];  /* optional text description, len 80+1 */
      int16       modified;     /* indicates file was modified */
  }
version_t;

/* record of a block of data descriptors, mirrors structure of a HDF file.  */
typedef struct ddblock_t
  {
      uintn       dirty;        /* boolean: should this DD block be flushed? */
      int32       myoffset;     /* offset of this DD block in the file */
      int16       ndds;         /* number of dd's in this block */
      int32       nextoffset;   /* offset to the next ddblock in the file */
      struct filerec_t *frec;   /* Pointer to the filerec this block is in */
      struct ddblock_t *next;   /* pointer to the next ddblock in memory */
      struct ddblock_t *prev;   /* Pointer to previous ddblock. */
      struct dd_t *ddlist;      /* pointer to array of dd's */
  }
ddblock_t;

/* Tag tree node structure */
typedef struct tag_info_str
  {
      uint16 tag;       /* tag value for this node */
      /* Needs to be first in this structure */
      bv_ptr b;         /* bit-vector to keep track of which refs are used */
      dynarr_p d;       /* dynarray of the refs for this tag */
  }tag_info;

/* For determining what the last file operation was */
typedef enum
  {
      H4_OP_UNKNOWN = 0,   /* Don't know what the last operation was (after fopen frex) */
      H4_OP_SEEK,          /* Last operation was a seek */
      H4_OP_WRITE,         /* Last operation was a write */
      H4_OP_READ           /* Last operation was a read */
  }
fileop_t;

/* File record structure */
typedef struct filerec_t
  {
      char       *path;         /* name of file */
      hdf_file_t  file;         /* either file descriptor or pointer */
      uint16      maxref;       /* highest ref in this file */
      intn        access;       /* access mode */
      intn        refcount;     /* reference count / times opened */
      intn        attach;       /* number of access elts attached */
      intn        version_set;  /* version tag stuff */
      version_t   version;      /* file version info */

      /* Seek caching info */
      int32      f_cur_off;    /* Current location in the file */
      fileop_t    last_op;      /* the last file operation performed */

      /* DD block caching info */
      intn        cache;        /* boolean: whether caching is on */
      intn        dirty;        /* boolean: if dd list needs to be flushed */
      int32      f_end_off;    /* offset of the end of the file */

      /* DD list pointers */
      struct ddblock_t *ddhead; /* head of ddblock list */
      struct ddblock_t *ddlast; /* end of ddblock list */

      /* NULL DD pointers (for fast lookup of DFTAG_NULL) */
      struct ddblock_t *ddnull; /* location of last ddblock with a DFTAG_NULL */
      int32       ddnull_idx;   /* offset of the last location with DFTAG_NULL */

      /* tag tree for file */
      TBBT_TREE *tag_tree;      /* TBBT of the tags in the file */

      /* annotation stuff for file */
      intn       an_num[4];   /* Holds number of annotations found of each type */
      TBBT_TREE *an_tree[4];  /* tbbt trees for each type of annotation in file 
                               * i.e. file/data labels and descriptions.
                               * This is done for faster searching of annotations
                               * of a particular type. */
  }
filerec_t;

/* bits for filerec_t 'dirty' flag */
#define DDLIST_DIRTY   0x01     /* mark whether to flush dirty DD blocks */
#define FILE_END_DIRTY 0x02     /* indicate that the file needs to be extended */

/* Each access element is associated with a tag/ref to keep track of
   the dd it is pointing at.  To facilitate searching for next dd's,
   instead of pointing directly to the dd, we point to the ddblock and
   index into the ddlist of that ddblock. */
typedef struct accrec_t
  {
      /* Flags for this access record */
      intn        appendable;   /* whether appends to the data are allowed */
      intn        special;      /* special element ? */
      intn        new_elem;     /* is a new element (i.e. no length set yet) */
      int32       block_size;   /* size of the blocks for linked-block element*/
      int32       num_blocks;   /* number blocks in the linked-block element */
      uint32      access;       /* access codes */
      uintn       access_type;  /* I/O access type: serial/parallel/... */
      int32       file_id;      /* id of attached file */
      atom_t      ddid;         /* DD id for the DD attached to */
      int32       posn;         /* seek position with respect to start of element */
      void *       special_info; /* special element info? */
      struct funclist_t *special_func;  /* ptr to special function? */
      struct accrec_t *next;    /* for free-list linking */
  }
accrec_t;

#ifdef HFILE_MASTER
/* Pointer to the access record node free list */
static accrec_t *accrec_free_list=NULL;
#endif /* HFILE_MASTER */

/* this type is returned to applications programs or other special
   interfaces when they need to know information about a given
   special element.  This is all information that would not be returned
   via Hinquire().  This should probably be a union of structures. */
/* Added length of external element.  Note: this length is not returned
   via Hinquire(). -BMR 2011/12/12 */
typedef struct sp_info_block_t
  {
      int16       key;          /* type of special element this is */

      /* external elements */
      int32       offset;       /* offset in the file */
      int32       length;       /* length of external data in the file */
      int32       length_file_name;  /* length of external file name */
      char       *path;         /* file name - should not be freed by user */

      /* linked blocks */
      int32       first_len;    /* length of first block */
      int32       block_len;    /* length of standard block */
      int32       nblocks;      /* number of blocks per chunk */

      /* compressed elements */
      int32       comp_type;    /* compression type */
      int32       model_type;   /* model type */
      int32       comp_size;    /* size of compressed information */

      /* variable-length linked blocks */
      int32       min_block;    /* the minimum block size */

    /* Chunked elements */
      int32       chunk_size;   /* logical size of chunks */
      int32       ndims;        /* number of dimensions */
      int32       *cdims;       /* array of chunk lengths for each dimension */

      /* Buffered elements */
      int32       buf_aid;      /* AID of element buffered */
  }
sp_info_block_t;

/* a function table record for accessing special data elements.
   special data elements of a key could be accessed through the list
   of functions in array pointed to by tab. */
typedef struct funclist_t
  {
      int32       (*stread) (accrec_t * rec);
      int32       (*stwrite) (accrec_t * rec);
      int32       (*seek) (accrec_t * access_rec, int32 offset, intn origin);
      int32       (*inquire) (accrec_t * access_rec, int32 *pfile_id,
                                 uint16 *ptag, uint16 *pref, int32 *plength,
                               int32 *poffset, int32 *pposn, int16 *paccess,
                                     int16 *pspecial);
      int32       (*read) (accrec_t * access_rec, int32 length, void * data);
      int32       (*write) (accrec_t * access_rec, int32 length, const void * data);
      intn        (*endaccess) (accrec_t * access_rec);
      int32       (*info) (accrec_t * access_rec, sp_info_block_t * info);
      int32       (*reset) (accrec_t * access_rec, sp_info_block_t * info);
  }
funclist_t;

typedef struct functab_t
  {
      int16       key;          /* the key for this type of special elt */
      funclist_t *tab;          /* table of accessing functions */
  }
functab_t;

/* ---------------------- ID Types and Manipulation ----------------------- */
/* each id, what ever the type, will be represented with a 32-bit word,
   the most-significant 16 bits is a number unique for type.  The less-
   significant 16 bits is an id unique to each type; in this, we use the
   internal slot number. */

#define FIDTYPE   1
#define AIDTYPE   2
#define GROUPTYPE 3
#define SDSTYPE   4
#define DIMTYPE   5
#define CDFTYPE   6
#define VGIDTYPE  8     /* also defined in vg.h for Vgroups */
#define VSIDTYPE  9     /* also defined in vg.h for Vsets */
#define BITTYPE   10    /* For bit-accesses */
#define GRIDTYPE  11    /* for GR access */
#define RIIDTYPE  12    /* for RI access */

#define BADFREC(r)  ((r)==NULL || (r)->refcount==0)

/* --------------------------- Special Elements --------------------------- */
/* The HDF tag space is divided as follows based on the 2 highest bits:
   00: NCSA reserved ordinary tags
   01: NCSA reserved special tags(e.g. linked-block, external, compressed,..)
   10, 11: User tags.

   It is relatively cheap to operate with special tags within the NCSA
   reserved tags range. For users to specify special tags and their
   corresponding ordinary tag, the pair has to be added to the
   special_table in hfile.c and SPECIAL_TABLE must be defined. */

#ifdef SPECIAL_TABLE
#define BASETAG(t)      (HDbase_tag(t))
#define SPECIALTAG(t)   (HDis_special_tag(t))
#define MKSPECIALTAG(t) (HDmake_special_tag(t))
#else
/* This macro converts a (potentially) special tag into a normal tag */
#define BASETAG(t)      (uint16)((~(t) & 0x8000) ? ((t) & ~0x4000) : (t))
/* This macro checks if a tag is special */
#define SPECIALTAG(t)   (uint16)((~(t) & 0x8000) && ((t) & 0x4000))
/* This macro (potentially) converts a regular tag into a special tag */
#define MKSPECIALTAG(t) (uint16)((~(t) & 0x8000) ? ((t) | 0x4000) : DFTAG_NULL)
#endif /*SPECIAL_TABLE */

/* -------------------------- H-Layer Prototypes -------------------------- */
/*
   ** Functions to get information of special elt from other access records.
   **   defined in hfile.c
   ** These should really be HD... routines, but they are only used within
   **   the H-layer...
 */

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

    HDFLIBAPI accrec_t *HIget_access_rec
                (void);

    HDFLIBAPI void HIrelease_accrec_node(accrec_t *acc);

    HDFLIBAPI void * HIgetspinfo
                (accrec_t * access_rec);

    HDFLIBAPI intn HPcompare_filerec_path
                (const void * obj, const void * key);

    HDFLIBAPI intn HPcompare_accrec_tagref
                (const void * rec1, const void * rec2);

    HDFLIBAPI int32 HPgetdiskblock
                (filerec_t * file_rec, int32 block_size, intn moveto);

    HDFLIBAPI intn HPfreediskblock
                (filerec_t * file_rec, int32 block_offset, int32 block_size);

    HDFLIBAPI intn HPisfile_in_use
                (const char *path);

    HDFLIBAPI int32 HDcheck_empty
                (int32 file_id, uint16 tag, uint16 ref, intn *emptySDS);

    HDFLIBAPI int32 HDget_special_info
                (int32 access_id, sp_info_block_t * info_block);

    HDFLIBAPI int32 HDset_special_info
                (int32 access_id, sp_info_block_t * info_block);

    HDFLIBAPI intn HP_read
                (filerec_t *file_rec,void * buf,int32 bytes);

    HDFLIBAPI intn HPseek
                (filerec_t *file_rec,int32 offset);

    HDFLIBAPI intn HP_write
                (filerec_t *file_rec,const void * buf,int32 bytes);

    HDFLIBAPI int32 HPread_drec
                (int32 file_id, atom_t data_id, uint8** drec_buf);

    HDFLIBAPI intn tagcompare
                (void * k1, void * k2, intn cmparg);

    HDFLIBAPI VOID tagdestroynode
                (void * n);

/*
   ** from hblocks.c
 */
    HDFLIBAPI int32 HLPstread
                (accrec_t * access_rec);

    HDFLIBAPI int32 HLPstwrite
                (accrec_t * access_rec);

    HDFLIBAPI int32 HLPseek
                (accrec_t * access_rec, int32 offset, int origin);

    HDFLIBAPI int32 HLPread
                (accrec_t * access_rec, int32 length, void * data);

    HDFLIBAPI int32 HLPwrite
                (accrec_t * access_rec, int32 length, const void * data);

    HDFLIBAPI int32 HLPinquire
                (accrec_t * access_rec, int32 *pfile_id, uint16 *ptag, uint16 *pref,
               int32 *plength, int32 *poffset, int32 *pposn, int16 *paccess,
                 int16 *pspecial);

    HDFLIBAPI intn HLPendaccess
                (accrec_t * access_rec);

    HDFLIBAPI int32 HLPcloseAID
                (accrec_t * access_rec);

    HDFLIBAPI int32 HLPinfo
                (accrec_t * access_rec, sp_info_block_t * info_block);

/*
   ** from hextelt.c
 */
    HDFLIBAPI int32 HXPstread
                (accrec_t * rec);

    HDFLIBAPI int32 HXPstwrite
                (accrec_t * rec);

    HDFLIBAPI int32 HXPseek
                (accrec_t * access_rec, int32 offset, int origin);

    HDFLIBAPI int32 HXPread
                (accrec_t * access_rec, int32 length, void * data);

    HDFLIBAPI int32 HXPwrite
                (accrec_t * access_rec, int32 length, const void * data);

    HDFLIBAPI int32 HXPinquire
                (accrec_t * access_rec, int32 *pfile_id, uint16 *ptag, uint16 *pref,
               int32 *plength, int32 *poffset, int32 *pposn, int16 *paccess,
                 int16 *pspecial);

    HDFLIBAPI intn HXPendaccess
                (accrec_t * access_rec);

    HDFLIBAPI int32 HXPcloseAID
                (accrec_t * access_rec);

    HDFLIBAPI int32 HXPinfo
                (accrec_t * access_rec, sp_info_block_t * info_block);

    HDFLIBAPI int32 HXPreset
                (accrec_t * access_rec, sp_info_block_t * info_block);

    HDFLIBAPI intn HXPsetaccesstype
                (accrec_t * access_rec);

    HDFLIBAPI intn HXPshutdown
                (void);

/*
   ** from hcomp.c
 */

    HDFLIBAPI int32 HCPstread
                (accrec_t * rec);

    HDFLIBAPI int32 HCPstwrite
                (accrec_t * rec);

    HDFLIBAPI int32 HCPseek
                (accrec_t * access_rec, int32 offset, int origin);

    HDFLIBAPI int32 HCPinquire
                (accrec_t * access_rec, int32 *pfile_id, uint16 *ptag, uint16 *pref,
               int32 *plength, int32 *poffset, int32 *pposn, int16 *paccess,
                 int16 *pspecial);

    HDFLIBAPI int32 HCPread
                (accrec_t * access_rec, int32 length, void * data);

    HDFLIBAPI int32 HCPwrite
                (accrec_t * access_rec, int32 length, const void * data);

    HDFLIBAPI intn HCPendaccess
                (accrec_t * access_rec);

    HDFLIBAPI int32 HCPcloseAID
                (accrec_t * access_rec);

    HDFLIBAPI int32 HCPinfo
                (accrec_t * access_rec, sp_info_block_t * info_block);

    HDFLIBAPI int32 get_comp_len
	        (accrec_t* access_rec);

/*
   ** from hchunks.c - should be the same as found in 'hchunks.h'
 */
#include "hchunks.h"


/*
   ** from hbuffer.c
 */

    HDFLIBAPI int32 HBPstread
                (accrec_t * rec);

    HDFLIBAPI int32 HBPstwrite
                (accrec_t * rec);

    HDFLIBAPI int32 HBPseek
                (accrec_t * access_rec, int32 offset, int origin);

    HDFLIBAPI int32 HBPinquire
                (accrec_t * access_rec, int32 *pfile_id, uint16 *ptag, uint16 *pref,
               int32 *plength, int32 *poffset, int32 *pposn, int16 *paccess,
                 int16 *pspecial);

    HDFLIBAPI int32 HBPread
                (accrec_t * access_rec, int32 length, void * data);

    HDFLIBAPI int32 HBPwrite
                (accrec_t * access_rec, int32 length, const void * data);

    HDFLIBAPI intn HBPendaccess
                (accrec_t * access_rec);

    HDFLIBAPI int32 HBPcloseAID
                (accrec_t * access_rec);

    HDFLIBAPI int32 HBPinfo
                (accrec_t * access_rec, sp_info_block_t * info_block);

/*
   ** from hcompri.c
 */

    HDFLIBAPI int32 HRPstread
                (accrec_t * rec);

    HDFLIBAPI int32 HRPstwrite
                (accrec_t * rec);

    HDFLIBAPI int32 HRPseek
                (accrec_t * access_rec, int32 offset, int origin);

    HDFLIBAPI int32 HRPinquire
                (accrec_t * access_rec, int32 *pfile_id, uint16 *ptag, uint16 *pref,
               int32 *plength, int32 *poffset, int32 *pposn, int16 *paccess,
                 int16 *pspecial);

    HDFLIBAPI int32 HRPread
                (accrec_t * access_rec, int32 length, void * data);

    HDFLIBAPI int32 HRPwrite
                (accrec_t * access_rec, int32 length, const void * data);

    HDFLIBAPI intn HRPendaccess
                (accrec_t * access_rec);

    HDFLIBAPI int32 HRPcloseAID
                (accrec_t * access_rec);

    HDFLIBAPI int32 HRPinfo
                (accrec_t * access_rec, sp_info_block_t * info_block);

/*
   ** from hfiledd.c
 */
/******************************************************************************
 NAME
     HTPstart - Initialize the DD list in memory

 DESCRIPTION
    Reads the DD blocks from disk and creates the in-memory structures for
    handling them.  This routine should only be called once for a given
    file and HTPend should be called when finished with the DD list (i.e.
    when the file is being closed).

 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn HTPstart(filerec_t *file_rec       /* IN:  File record to store info in */
);

/******************************************************************************
 NAME
     HTPinit - Create a new DD list in memory

 DESCRIPTION
    Creates a new DD list in memory for a newly created file.  This routine
    should only be called once for a given file and HTPend should be called
    when finished with the DD list (i.e.  when the file is being closed).

 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn HTPinit(filerec_t *file_rec,       /* IN: File record to store info in */
    int16 ndds                          /* IN: # of DDs to store in each block */
);

/******************************************************************************
 NAME
     HTPsync - Flush the DD list in memory

 DESCRIPTION
    Syncronizes the in-memory copy of the DD list with the copy on disk by
    writing out the DD blocks which have changed to disk.
    
 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn HTPsync(filerec_t *file_rec       /* IN:  File record to store info in */
);

/******************************************************************************
 NAME
     HTPend - Terminate the DD list in memory

 DESCRIPTION
    Terminates access to the DD list in memory, writing the DD blocks out to
    the disk (if they've changed).  After this routine is called, no further
    access to tag/refs (or essentially any other HDF objects) can be performed
    on the file.

 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn HTPend(filerec_t *file_rec       /* IN:  File record to store info in */
); 

/******************************************************************************
 NAME
     HTPcreate - Create (and attach to) a tag/ref pair

 DESCRIPTION
    Creates a new tag/ref pair in memory and inserts the tag/ref pair into the
    DD list to be written out to disk.  This routine returns a DD id which can
    be used in the other tag/ref routines to modify the DD.

 RETURNS
    Returns DD id if successful and FAIL otherwise

*******************************************************************************/
atom_t HTPcreate(filerec_t *file_rec,   /* IN: File record to store info in */
    uint16 tag,                         /* IN: Tag to create */
    uint16 ref                          /* IN: ref to create */
);

/******************************************************************************
 NAME
     HTPselect - Attach to an existing tag/ref pair

 DESCRIPTION
    Attaches to an existing tag/ref pair.  This routine returns a DD id which
    can be used in the other tag/ref routines to modify the DD.

 RETURNS
    Returns DD id if successful and FAIL otherwise

*******************************************************************************/
atom_t HTPselect(filerec_t *file_rec,   /* IN: File record to store info in */
    uint16 tag,                         /* IN: Tag to select */
    uint16 ref                          /* IN: ref to select */
);

/******************************************************************************
 NAME
     HTPendaccess - End access to an existing tag/ref pair

 DESCRIPTION
    Ends access to an existing tag/ref pair.  Any further access to the tag/ref
    pair may result in incorrect information being recorded about the DD in
    memory or on disk.

 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn HTPendaccess(atom_t ddid           /* IN: DD id to end access to */
);

/******************************************************************************
 NAME
     HTPdelete - Delete an existing tag/ref pair

 DESCRIPTION
    Deletes a tag/ref from the file.  Also ends access to the tag/ref pair.
    Any further access to the tag/ref pair may result in incorrect information
    being recorded about the DD in memory or on disk.

 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn HTPdelete(atom_t ddid              /* IN: DD id to delete */
);

/******************************************************************************
 NAME
     HTPupdate - Change the offset or length of an existing tag/ref pair

 DESCRIPTION
    Updates a tag/ref in the file, allowing the length and/or offset to be
    modified. Note: "INVALID_OFFSET" & "INVALID_LENGTH" are used to indicate
    that the length or offset (respectively) is unchanged.

 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn HTPupdate(atom_t ddid,             /* IN: DD id to update */
    int32 new_off,                      /* IN: new offset for DD */
    int32 new_len                       /* IN: new length for DD */
);

/******************************************************************************
 NAME
     HTPinquire - Get the DD information for a DD (i.e. tag/ref/offset/length)

 DESCRIPTION
    Get the DD information for a DD id from the DD block.  Passing NULL for
    any parameter does not try to update that parameter.

 RETURNS
    Returns SUCCEED if successful and FAIL otherwise

*******************************************************************************/
intn HTPinquire(atom_t ddid,            /* IN: DD id to inquire about */
    uint16 *tag,                        /* IN: tag of DD */
    uint16 *ref,                        /* IN: ref of DD */
    int32 *off,                         /* IN: offset of DD */
    int32 *len                          /* IN: length of DD */
);

/******************************************************************************
 NAME
     HTPis_special - Check if a DD id is associated with a special tag

 DESCRIPTION
    Checks if the tag for the DD id is a special tag.

 RETURNS
    Returns TRUE(1)/FALSE(0) if successful and FAIL otherwise

*******************************************************************************/
intn HTPis_special(atom_t ddid             /* IN: DD id to inquire about */
);

/******************************************************************************
 NAME
    HTPdump_dds -- Dump out the dd information for a file

 DESCRIPTION
    Prints out all the information (that you could _ever_ want to know) about
    the dd blocks and dd list for a file.

 RETURNS
    returns SUCCEED (0) if successful and FAIL (-1) if failed.

*******************************************************************************/
intn HTPdump_dds(int32 file_id,     /* IN: file ID of HDF file to dump info for */
    FILE *fout                      /* IN: file stream to output to */
);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

/* #define DISKBLOCK_DEBUG */
#ifdef DISKBLOCK_DEBUG

#ifndef HFILE_MASTER
extern
#endif /* HFILE_MASTER */
const uint8 diskblock_header[4]
#ifdef HFILE_MASTER
={0xde, 0xad, 0xbe, 0xef}
#endif /* HFILE_MASTER */
;

#ifndef HFILE_MASTER
extern
#endif /* HFILE_MASTER */
const uint8 diskblock_tail[4]
#ifdef HFILE_MASTER
={0xfe, 0xeb, 0xda, 0xed}
#endif /* HFILE_MASTER */
;
#define DISKBLOCK_HSIZE sizeof(diskblock_header)
#define DISKBLOCK_TSIZE sizeof(diskblock_tail)

#endif /* DISKBLOCK_DEBUG */

#endif                          /* HFILE_H */
