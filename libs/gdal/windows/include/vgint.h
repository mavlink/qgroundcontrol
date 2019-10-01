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

/*****************************************************************************
*
* vgint.h
*
* Part of HDF VSet interface
*
* defines library private symbols and structures used in v*.c files
*
* NOTES:
* This include file depends on the basic HDF *.h files hdfi.h and hdf.h.
* An 'S' in the comment means that that data field is saved in the HDF file.
*
******************************************************************************/

#ifndef _VGINT_H
#define _VGINT_H

#include "H4api_adpt.h"

#include "hfile.h"

/* Include file for Threaded, Balanced Binary Tree implementation */
#include "tbbt.h"

/*
 * typedefs for VGROUP, VDATA and VSUBGROUP
 */
typedef struct vgroup_desc VGROUP;
typedef struct vdata_desc VDATA;
typedef VDATA VSUBGROUP;

/*
 * -----------------------------------------------------------------
 * structures that are part of the VDATA structure
 * -----------------------------------------------------------------
 */

typedef struct symdef_struct
  {
      char    *name;         /* symbol name */
      int16    type;         /* whether int, char, float etc */
      uint16   isize;        /* field size as stored in vdata */
      uint16   order;        /* order of field */
  }
SYMDEF;

typedef struct write_struct
  {
      intn     n;            /* S actual # fields in element */
      uint16   ivsize;       /* S size of element as stored in vdata */

      char     name[VSFIELDMAX][FIELDNAMELENMAX + 1]; /* S name of each field */

      int16    len[VSFIELDMAX];   /* S length of each fieldname */
      int16    type[VSFIELDMAX];  /* S field type */
      uint16   off[VSFIELDMAX];   /* S field offset in element in vdata */
      uint16   isize[VSFIELDMAX]; /* S internal (HDF) size [incl order] */
      uint16   order[VSFIELDMAX]; /* S order of field */
      uint16   esize[VSFIELDMAX]; /*  external (local machine) size [incl order] */
  }
VWRITELIST;

typedef struct dyn_write_struct
  {
      intn     n;      /* S actual # fields in element */
      uint16   ivsize; /* S size of element as stored in vdata */
      char     **name; /* S name of each field */
#ifndef OLD_WAY
      uint16   *bptr;  /* Pointer to hold the beginning of the buffer */
#endif /* OLD_WAY */
      int16    *type;  /* S field type (into bptr buffer) */
      uint16   *off;   /* S field offset in element in vdata (into bptr buffer) */
      uint16   *isize; /* S internal (HDF) size [incl order] (into bptr buffer) */
      uint16   *order; /* S order of field (into bptr buffer) */
      uint16   *esize; /* external (local machine) size [incl order] (into bptr buffer) */
  }
DYN_VWRITELIST;

/* If there are too many attrs and performance becomes a problem,
   the vs_attr_t list defined below can be replaced by an
   array of attr lists, each list contains attrs for 1 field.
 */ 
typedef struct dyn_vsattr_struct
{
      int32    findex;     /* which field this attr belongs to */
      uint16   atag, aref; /* tag/ref pair of the attr     */
} vs_attr_t;

typedef struct dyn_vgattr_struct
{
      uint16   atag, aref; /* tag/ref pair of the attr     */
} vg_attr_t;

typedef struct dyn_read_struct
{
      intn      n;         /* # fields to read */
      intn      *item;     /* index into vftable_struct */
} DYN_VREADLIST;

/*
 *  -----------------------------------------------
 *         V G R O U P     definition
 *  -----------------------------------------------
 */

struct vgroup_desc
  {
      uint16      otag, oref;   /* tag-ref of this vgroup */
      HFILEID     f;            /* HDF file id  */
      uint16      nvelt;        /* S no of objects */
      intn        access;       /* 'r' or 'w' */
      uint16     *tag;          /* S tag of objects */
      uint16     *ref;          /* S ref of objects */
      char       *vgname;       /* S name of this vgroup */
      char       *vgclass;      /* S class name of this vgroup */
      intn        marked;       /* =1 if new info has been added to vgroup */
      intn        new_vg;       /* =1 if this is a new Vgroup */
      uint16      extag, exref; /* expansion tag-ref */
      intn        msize;        /* max size of storage arrays */
      uint32      flags;        /* indicate which version of VG should
                                   be written to the file */
      int32       nattrs;       /* number of attributes */
      vg_attr_t  *alist;        /* index of new-style attributes, by Vsetattr */
      int32       noldattrs;    /* number of old-style attributes */
      vg_attr_t  *old_alist;    /* refs of attributes - only used in memory to
				prevent repeated code in making the list; see
				Voldnattrs's header for details -BMR 2/4/2011 */
      vg_attr_t  *all_alist;    /* combined list; previous approach, only keep
				just in case we come back to that approach; will
				remove it once we decide not to go back 2/16/11 */
      int16       version, more; /* version and "more" field */
      struct vgroup_desc *next; /* pointer to next node (for free list only) */
  };
/* VGROUP */

/*
 *  -----------------------------------------------
 *         V D A T A      definition
 *  -----------------------------------------------
 */

struct vdata_desc
  {
      uint16      otag, oref;   /* tag,ref of this vdata */
      HFILEID     f;            /* HDF file id */
      intn        access;       /* 'r' or 'w' */
      char        vsname[VSNAMELENMAX + 1];     /* S name of this vdata */
      char        vsclass[VSNAMELENMAX + 1];    /* S class name of this vdata */
      int16       interlace;    /* S  interlace as in file */
      int32       nvertices;    /* S  #vertices in this vdata */
      DYN_VWRITELIST  wlist;
      DYN_VREADLIST   rlist;
      int16       nusym;
      SYMDEF      *usym;
      intn        marked;       /* =1 if new info has been added to vdata */
      intn        new_h_sz;     /* =1 if VH size changed, due to new attrs etc. */
      intn        islinked;     /* =1 if vdata is a linked-block in file */

      uint16      extag, exref; /* expansion tag-ref */
      uint32      flags;     /* bit 0 -- has attr
                                bit 1 -- "large field"
                                bit 2 -- "interlaced data is appendable"
                                bit 3-15  -- unused.   */
      intn        nattrs;
      vs_attr_t   *alist;    /* attribute list */
      int16       version, more;    /* version and "more" field */
      int32       aid;          /* access id - for LINKED blocks */
      struct vs_instance_struct *instance;  /* ptr to the intance struct for this VData */
      struct vdata_desc *next;  /* pointer to next node (for free list only) */
  };                            /* VDATA */

/* .................................................................. */
/* Private data structures. Unlikely to be of interest to applications */
/*
   * These are just typedefs. Actual vfile_ts are declared PRIVATE and
   * are not accessible by applications. However, you may change VFILEMAX
   * to allow however many files to be opened.
   *
   * These are memory-resident copies of the tag-refs of the vgroups
   * and vdatas for each file that is opened.
   *
 */

/* this is a memory copy of a vg tag/ref found in the file */
typedef struct vg_instance_struct
  {
      int32       key;          /* key to look up with the B-tree routines */
      /* needs to be first in the structure */
      uintn       ref;          /* ref # of this vgroup in the file */
      /* needs to be second in the structure */
      intn        nattach;      /* # of current attachs to this vgroup */
      int32       nentries;     /* # of entries in that vgroup initially */
      VGROUP     *vg;           /* points to the vg when it is attached */
      struct vg_instance_struct *next;  /* pointer to next node (for free list only) */
  }
vginstance_t;

/* this is a memory copy of a vs tag/ref found in the file */
typedef struct vs_instance_struct
  {
      int32       key;          /* key to look up with the B-tree routines */
      /* needs to be first in the structure */
      uintn       ref;          /* ref # of this vdata in the file */
      /* needs to be second in the structure */
      intn        nattach;      /* # of current attachs to this vdata */
      int32       nvertices;    /* # of elements in that vdata initially */
      VDATA      *vs;           /* points to the vdata when it is attached */
      struct vs_instance_struct *next;  /* pointer to next node (for free list only) */
  }
vsinstance_t;

/* each vfile_t maintains 2 linked lists: one of vgs and one of vdatas
 * that already exist or are just created for a given file.  */
typedef struct vfiledir_struct
  {
      int32            f;       /* HDF File ID */

      int32       vgtabn;       /* # of vg entries in vgtab so far */
      TBBT_TREE  *vgtree;       /* Root of VGroup B-Tree */

      int32       vstabn;       /* # of vs entries in vstab so far */
      TBBT_TREE  *vstree;       /* Root of VSet B-Tree */
      intn        access;       /* the number of active pointers to this file's Vstuff */
  }
vfile_t;

/* .................................................................. */

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/*
 * Library private routines for the VSet layer
 */
    VDATA *VSIget_vdata_node(void);

    void VSIrelease_vdata_node(VDATA *v);

    intn VSIgetvdatas(int32 id, const char *vsclass, const uintn start_vd,
	const uintn n_vds, uint16 *refarray);

    HDFLIBAPI vsinstance_t *VSIget_vsinstance_node(void);

    HDFLIBAPI void VSIrelease_vsinstance_node(vsinstance_t *vs);

    VGROUP *VIget_vgroup_node(void);

    void VIrelease_vgroup_node(VGROUP *v);

    HDFLIBAPI vginstance_t *VIget_vginstance_node(void);

    HDFLIBAPI void VIrelease_vginstance_node(vginstance_t *vg);

    HDFLIBAPI intn VPparse_shutdown(void);

    HDFLIBAPI vfile_t *Get_vfile(HFILEID f);

    HDFLIBAPI vsinstance_t *vsinst
                (HFILEID f, uint16 vsid);

    HDFLIBAPI vginstance_t *vginst
            (HFILEID f, uint16 vgid);

    HDFLIBAPI DYN_VWRITELIST *vswritelist
                (int32 vskey);

    HDFLIBAPI intn vpackvg
                (VGROUP * vg, uint8 buf[], int32 * size);

    HDFLIBAPI int32 vinsertpair
                (VGROUP * vg, uint16 tag, uint16 ref);

    HDFLIBAPI intn vpackvs
                (VDATA * vs, uint8 buf[], int32 * size);

    HDFLIBAPI VGROUP *VPgetinfo
                (HFILEID f,uint16 ref);

    HDFLIBAPI VDATA *VSPgetinfo
                (HFILEID f,uint16 ref);

    HDFLIBAPI int16 map_from_old_types
                (intn type);

    HDFLIBAPI void trimendblanks
                (char *ss);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

#endif                          /* _VGINT_H */
