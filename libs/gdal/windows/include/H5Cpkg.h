/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer: John Mainzer -- 10/12/04
 *
 * Purpose:     This file contains declarations which are normally visible
 *              only within the H5C package.
 *
 *		Source files outside the H5C package should include
 *		H5Cprivate.h instead.
 *
 *		The one exception to this rule is test/cache.c.  The test
 *		code is easier to write if it can look at the cache's
 *		internal data structures.  Indeed, this is the main
 *		reason why this file was created.
 */

#if !(defined H5C_FRIEND || defined H5C_MODULE)
#error "Do not include this file outside the H5C package!"
#endif

#ifndef _H5Cpkg_H
#define _H5Cpkg_H

/* Get package's private header */
#include "H5Cprivate.h"

/* Other private headers needed by this file */
#include "H5SLprivate.h"        /* Skip lists */

/**************************/
/* Package Private Macros */
/**************************/

/* Number of epoch markers active */
#define H5C__MAX_EPOCH_MARKERS                  10

/* Cache configuration settings */
#define H5C__HASH_TABLE_LEN     (64 * 1024) /* must be a power of 2 */
#define H5C__H5C_T_MAGIC	0x005CAC0E

/* Initial allocated size of the "flush_dep_parent" array */
#define H5C_FLUSH_DEP_PARENT_INIT 8

/****************************************************************************
 *
 * We maintain doubly linked lists of instances of H5C_cache_entry_t for a
 * variety of reasons -- protected list, LRU list, and the clean and dirty
 * LRU lists at present.  The following macros support linking and unlinking
 * of instances of H5C_cache_entry_t by both their regular and auxiliary next
 * and previous pointers.
 *
 * The size and length fields are also maintained.
 *
 * Note that the relevant pair of prev and next pointers are presumed to be
 * NULL on entry in the insertion macros.
 *
 * Finally, observe that the sanity checking macros evaluate to the empty
 * string when H5C_DO_SANITY_CHECKS is FALSE.  They also contain calls
 * to the HGOTO_ERROR macro, which may not be appropriate in all cases.
 * If so, we will need versions of the insertion and deletion macros which
 * do not reference the sanity checking macros.
 *							JRM - 5/5/04
 *
 * Changes:
 *
 *  - Removed the line:
 *
 *        ( ( (Size) == (entry_ptr)->size ) && ( (len) != 1 ) ) ||
 *
 *    from the H5C__DLL_PRE_REMOVE_SC macro.  With the addition of the
 *    epoch markers used in the age out based cache size reduction algorithm,
 *    this invariant need not hold, as the epoch markers are of size 0.
 *
 *    One could argue that I should have given the epoch markers a positive
 *    size, but this would break the index_size = LRU_list_size + pl_size
 *    + pel_size invariant.
 *
 *    Alternatively, I could pass the current decr_mode in to the macro,
 *    and just skip the check whenever epoch markers may be in use.
 *
 *    However, any size errors should be caught when the cache is flushed
 *    and destroyed.  Until we are tracking such an error, this should be
 *    good enough.
 *                                                     JRM - 12/9/04
 *
 *
 *  - In the H5C__DLL_PRE_INSERT_SC macro, replaced the lines:
 *
 *    ( ( (len) == 1 ) &&
 *      ( ( (head_ptr) != (tail_ptr) ) || ( (Size) <= 0 ) ||
 *        ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )
 *      )
 *    ) ||
 *
 *    with:
 *
 *    ( ( (len) == 1 ) &&
 *      ( ( (head_ptr) != (tail_ptr) ) ||
 *        ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )
 *      )
 *    ) ||
 *
 *    Epoch markers have size 0, so we can now have a non-empty list with
 *    zero size.  Hence the "( (Size) <= 0 )" clause cause false failures
 *    in the sanity check.  Since "Size" is typically a size_t, it can't
 *    take on negative values, and thus the revised clause "( (Size) < 0 )"
 *    caused compiler warnings.
 *                                                     JRM - 12/22/04
 *
 *  - In the H5C__DLL_SC macro, replaced the lines:
 *
 *    ( ( (len) == 1 ) &&
 *      ( ( (head_ptr) != (tail_ptr) ) || ( (cache_ptr)->size <= 0 ) ||
 *        ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )
 *      )
 *    ) ||
 *
 *    with
 *
 *    ( ( (len) == 1 ) &&
 *      ( ( (head_ptr) != (tail_ptr) ) ||
 *        ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )
 *      )
 *    ) ||
 *
 *    Epoch markers have size 0, so we can now have a non-empty list with
 *    zero size.  Hence the "( (Size) <= 0 )" clause cause false failures
 *    in the sanity check.  Since "Size" is typically a size_t, it can't
 *    take on negative values, and thus the revised clause "( (Size) < 0 )"
 *    caused compiler warnings.
 *                                                     JRM - 1/10/05
 *
 *  - Added the H5C__DLL_UPDATE_FOR_SIZE_CHANGE macro and the associated
 *    sanity checking macros.  These macro are used to update the size of
 *    a DLL when one of its entries changes size.
 *
 *							JRM - 9/8/05
 *
 *  - Added macros supporting the index list -- a doubly liked list of 
 *    all entries in the index.  This list is necessary to reduce the 
 *    cost of visiting all entries in the cache, which was previously
 *    done via a scan of the hash table.
 *
 *							JRM - 10/15/15
 *
 ****************************************************************************/

#if H5C_DO_SANITY_CHECKS

#define H5C__DLL_PRE_REMOVE_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv) \
if ( ( (head_ptr) == NULL ) ||                                               \
     ( (tail_ptr) == NULL ) ||                                               \
     ( (entry_ptr) == NULL ) ||                                              \
     ( (len) <= 0 ) ||                                                       \
     ( (Size) < (entry_ptr)->size ) ||                                       \
     ( ( (entry_ptr)->prev == NULL ) && ( (head_ptr) != (entry_ptr) ) ) ||   \
     ( ( (entry_ptr)->next == NULL ) && ( (tail_ptr) != (entry_ptr) ) ) ||   \
     ( ( (len) == 1 ) &&                                                     \
       ( ! ( ( (head_ptr) == (entry_ptr) ) &&                                \
             ( (tail_ptr) == (entry_ptr) ) &&                                \
             ( (entry_ptr)->next == NULL ) &&                                \
             ( (entry_ptr)->prev == NULL ) &&                                \
             ( (Size) == (entry_ptr)->size )                                 \
           )                                                                 \
       )                                                                     \
     )                                                                       \
   ) {                                                                       \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "DLL pre remove SC failed")     \
}

#define H5C__DLL_SC(head_ptr, tail_ptr, len, Size, fv)                   \
if ( ( ( ( (head_ptr) == NULL ) || ( (tail_ptr) == NULL ) ) &&           \
       ( (head_ptr) != (tail_ptr) )                                      \
     ) ||                                                                \
     ( (len) < 0 ) ||                                                    \
     ( (Size) < 0 ) ||                                                   \
     ( ( (len) == 1 ) &&                                                 \
       ( ( (head_ptr) != (tail_ptr) ) ||                                 \
         ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )        \
       )                                                                 \
     ) ||                                                                \
     ( ( (len) >= 1 ) &&                                                 \
       ( ( (head_ptr) == NULL ) || ( (head_ptr)->prev != NULL ) ||       \
         ( (tail_ptr) == NULL ) || ( (tail_ptr)->next != NULL )          \
       )                                                                 \
     )                                                                   \
   ) {                                                                   \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "DLL sanity check failed")  \
}

#define H5C__DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv) \
if ( ( (entry_ptr) == NULL ) ||                                              \
     ( (entry_ptr)->next != NULL ) ||                                        \
     ( (entry_ptr)->prev != NULL ) ||                                        \
     ( ( ( (head_ptr) == NULL ) || ( (tail_ptr) == NULL ) ) &&               \
       ( (head_ptr) != (tail_ptr) )                                          \
     ) ||                                                                    \
     ( ( (len) == 1 ) &&                                                     \
       ( ( (head_ptr) != (tail_ptr) ) ||                                     \
         ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )            \
       )                                                                     \
     ) ||                                                                    \
     ( ( (len) >= 1 ) &&                                                     \
       ( ( (head_ptr) == NULL ) || ( (head_ptr)->prev != NULL ) ||           \
         ( (tail_ptr) == NULL ) || ( (tail_ptr)->next != NULL )              \
       )                                                                     \
     )                                                                       \
   ) {                                                                       \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "DLL pre insert SC failed")     \
}

#define H5C__DLL_PRE_SIZE_UPDATE_SC(dll_len, dll_size, old_size, new_size)    \
if ( ( (dll_len) <= 0 ) ||                                                    \
     ( (dll_size) <= 0 ) ||                                                   \
     ( (old_size) <= 0 ) ||                                                   \
     ( (old_size) > (dll_size) ) ||                                           \
     ( (new_size) <= 0 ) ||                                                   \
     ( ( (dll_len) == 1 ) && ( (old_size) != (dll_size) ) ) ) {               \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "DLL pre size update SC failed") \
}

#define H5C__DLL_POST_SIZE_UPDATE_SC(dll_len, dll_size, old_size, new_size)    \
if ( ( (new_size) > (dll_size) ) ||                                            \
     ( ( (dll_len) == 1 ) && ( (new_size) != (dll_size) ) ) ) {                \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "DLL post size update SC failed") \
}

#else /* H5C_DO_SANITY_CHECKS */

#define H5C__DLL_PRE_REMOVE_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv)
#define H5C__DLL_SC(head_ptr, tail_ptr, len, Size, fv)
#define H5C__DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv)
#define H5C__DLL_PRE_SIZE_UPDATE_SC(dll_len, dll_size, old_size, new_size)
#define H5C__DLL_POST_SIZE_UPDATE_SC(dll_len, dll_size, old_size, new_size)

#endif /* H5C_DO_SANITY_CHECKS */


#define H5C__DLL_APPEND(entry_ptr, head_ptr, tail_ptr, len, Size, fail_val) \
{                                                                           \
    H5C__DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size,        \
                           fail_val)                                        \
    if ( (head_ptr) == NULL )                                               \
    {                                                                       \
       (head_ptr) = (entry_ptr);                                            \
       (tail_ptr) = (entry_ptr);                                            \
    }                                                                       \
    else                                                                    \
    {                                                                       \
       (tail_ptr)->next = (entry_ptr);                                      \
       (entry_ptr)->prev = (tail_ptr);                                      \
       (tail_ptr) = (entry_ptr);                                            \
    }                                                                       \
    (len)++;                                                                \
    (Size) += (entry_ptr)->size;                                            \
} /* H5C__DLL_APPEND() */

#define H5C__DLL_PREPEND(entry_ptr, head_ptr, tail_ptr, len, Size, fail_val) \
{                                                                            \
    H5C__DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size,         \
                           fail_val)                                         \
    if ( (head_ptr) == NULL )                                                \
    {                                                                        \
       (head_ptr) = (entry_ptr);                                             \
       (tail_ptr) = (entry_ptr);                                             \
    }                                                                        \
    else                                                                     \
    {                                                                        \
       (head_ptr)->prev = (entry_ptr);                                       \
       (entry_ptr)->next = (head_ptr);                                       \
       (head_ptr) = (entry_ptr);                                             \
    }                                                                        \
    (len)++;                                                                 \
    (Size) += entry_ptr->size;                                               \
} /* H5C__DLL_PREPEND() */

#define H5C__DLL_REMOVE(entry_ptr, head_ptr, tail_ptr, len, Size, fail_val) \
{                                                                           \
    H5C__DLL_PRE_REMOVE_SC(entry_ptr, head_ptr, tail_ptr, len, Size,        \
                           fail_val)                                        \
    {                                                                       \
       if ( (head_ptr) == (entry_ptr) )                                     \
       {                                                                    \
          (head_ptr) = (entry_ptr)->next;                                   \
          if ( (head_ptr) != NULL )                                         \
             (head_ptr)->prev = NULL;                                       \
       }                                                                    \
       else                                                                 \
          (entry_ptr)->prev->next = (entry_ptr)->next;                      \
       if ( (tail_ptr) == (entry_ptr) )                                     \
       {                                                                    \
          (tail_ptr) = (entry_ptr)->prev;                                   \
          if ( (tail_ptr) != NULL )                                         \
             (tail_ptr)->next = NULL;                                       \
       }                                                                    \
       else                                                                 \
          (entry_ptr)->next->prev = (entry_ptr)->prev;                      \
       entry_ptr->next = NULL;                                              \
       entry_ptr->prev = NULL;                                              \
       (len)--;                                                             \
       (Size) -= entry_ptr->size;                                           \
    }                                                                       \
} /* H5C__DLL_REMOVE() */

#define H5C__DLL_UPDATE_FOR_SIZE_CHANGE(dll_len, dll_size, old_size, new_size) \
{                                                                              \
    H5C__DLL_PRE_SIZE_UPDATE_SC(dll_len, dll_size, old_size, new_size)         \
    (dll_size) -= (old_size);                                                  \
    (dll_size) += (new_size);                                                  \
    H5C__DLL_POST_SIZE_UPDATE_SC(dll_len, dll_size, old_size, new_size)        \
} /* H5C__DLL_UPDATE_FOR_SIZE_CHANGE() */

#if H5C_DO_SANITY_CHECKS

#define H5C__AUX_DLL_PRE_REMOVE_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv) \
if ( ( (hd_ptr) == NULL ) ||                                                   \
     ( (tail_ptr) == NULL ) ||                                                 \
     ( (entry_ptr) == NULL ) ||                                                \
     ( (len) <= 0 ) ||                                                         \
     ( (Size) < (entry_ptr)->size ) ||                                         \
     ( ( (Size) == (entry_ptr)->size ) && ( ! ( (len) == 1 ) ) ) ||            \
     ( ( (entry_ptr)->aux_prev == NULL ) && ( (hd_ptr) != (entry_ptr) ) ) ||   \
     ( ( (entry_ptr)->aux_next == NULL ) && ( (tail_ptr) != (entry_ptr) ) ) || \
     ( ( (len) == 1 ) &&                                                       \
       ( ! ( ( (hd_ptr) == (entry_ptr) ) && ( (tail_ptr) == (entry_ptr) ) &&   \
             ( (entry_ptr)->aux_next == NULL ) &&                              \
             ( (entry_ptr)->aux_prev == NULL ) &&                              \
             ( (Size) == (entry_ptr)->size )                                   \
           )                                                                   \
       )                                                                       \
     )                                                                         \
   ) {                                                                         \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "aux DLL pre remove SC failed")   \
}

#define H5C__AUX_DLL_SC(head_ptr, tail_ptr, len, Size, fv)                  \
if ( ( ( ( (head_ptr) == NULL ) || ( (tail_ptr) == NULL ) ) &&              \
       ( (head_ptr) != (tail_ptr) )                                         \
     ) ||                                                                   \
     ( (len) < 0 ) ||                                                       \
     ( (Size) < 0 ) ||                                                      \
     ( ( (len) == 1 ) &&                                                    \
       ( ( (head_ptr) != (tail_ptr) ) || ( (Size) <= 0 ) ||                 \
         ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )           \
       )                                                                    \
     ) ||                                                                   \
     ( ( (len) >= 1 ) &&                                                    \
       ( ( (head_ptr) == NULL ) || ( (head_ptr)->aux_prev != NULL ) ||      \
         ( (tail_ptr) == NULL ) || ( (tail_ptr)->aux_next != NULL )         \
       )                                                                    \
     )                                                                      \
   ) {                                                                      \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "AUX DLL sanity check failed") \
}

#define H5C__AUX_DLL_PRE_INSERT_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv) \
if ( ( (entry_ptr) == NULL ) ||                                                \
     ( (entry_ptr)->aux_next != NULL ) ||                                      \
     ( (entry_ptr)->aux_prev != NULL ) ||                                      \
     ( ( ( (hd_ptr) == NULL ) || ( (tail_ptr) == NULL ) ) &&                   \
       ( (hd_ptr) != (tail_ptr) )                                              \
     ) ||                                                                      \
     ( ( (len) == 1 ) &&                                                       \
       ( ( (hd_ptr) != (tail_ptr) ) || ( (Size) <= 0 ) ||                      \
         ( (hd_ptr) == NULL ) || ( (hd_ptr)->size != (Size) )                  \
       )                                                                       \
     ) ||                                                                      \
     ( ( (len) >= 1 ) &&                                                       \
       ( ( (hd_ptr) == NULL ) || ( (hd_ptr)->aux_prev != NULL ) ||             \
         ( (tail_ptr) == NULL ) || ( (tail_ptr)->aux_next != NULL )            \
       )                                                                       \
     )                                                                         \
   ) {                                                                         \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "AUX DLL pre insert SC failed")   \
}

#else /* H5C_DO_SANITY_CHECKS */

#define H5C__AUX_DLL_PRE_REMOVE_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv)
#define H5C__AUX_DLL_SC(head_ptr, tail_ptr, len, Size, fv)
#define H5C__AUX_DLL_PRE_INSERT_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv)

#endif /* H5C_DO_SANITY_CHECKS */


#define H5C__AUX_DLL_APPEND(entry_ptr, head_ptr, tail_ptr, len, Size, fail_val)\
{                                                                              \
    H5C__AUX_DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size,       \
                               fail_val)                                       \
    if ( (head_ptr) == NULL )                                                  \
    {                                                                          \
       (head_ptr) = (entry_ptr);                                               \
       (tail_ptr) = (entry_ptr);                                               \
    }                                                                          \
    else                                                                       \
    {                                                                          \
       (tail_ptr)->aux_next = (entry_ptr);                                     \
       (entry_ptr)->aux_prev = (tail_ptr);                                     \
       (tail_ptr) = (entry_ptr);                                               \
    }                                                                          \
    (len)++;                                                                   \
    (Size) += entry_ptr->size;                                                 \
} /* H5C__AUX_DLL_APPEND() */

#define H5C__AUX_DLL_PREPEND(entry_ptr, head_ptr, tail_ptr, len, Size, fv)   \
{                                                                            \
    H5C__AUX_DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv) \
    if ( (head_ptr) == NULL )                                                \
    {                                                                        \
       (head_ptr) = (entry_ptr);                                             \
       (tail_ptr) = (entry_ptr);                                             \
    }                                                                        \
    else                                                                     \
    {                                                                        \
       (head_ptr)->aux_prev = (entry_ptr);                                   \
       (entry_ptr)->aux_next = (head_ptr);                                   \
       (head_ptr) = (entry_ptr);                                             \
    }                                                                        \
    (len)++;                                                                 \
    (Size) += entry_ptr->size;                                               \
} /* H5C__AUX_DLL_PREPEND() */

#define H5C__AUX_DLL_REMOVE(entry_ptr, head_ptr, tail_ptr, len, Size, fv)    \
{                                                                            \
    H5C__AUX_DLL_PRE_REMOVE_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv) \
    {                                                                        \
       if ( (head_ptr) == (entry_ptr) )                                      \
       {                                                                     \
          (head_ptr) = (entry_ptr)->aux_next;                                \
          if ( (head_ptr) != NULL )                                          \
             (head_ptr)->aux_prev = NULL;                                    \
       }                                                                     \
       else                                                                  \
          (entry_ptr)->aux_prev->aux_next = (entry_ptr)->aux_next;           \
       if ( (tail_ptr) == (entry_ptr) )                                      \
       {                                                                     \
          (tail_ptr) = (entry_ptr)->aux_prev;                                \
          if ( (tail_ptr) != NULL )                                          \
             (tail_ptr)->aux_next = NULL;                                    \
       }                                                                     \
       else                                                                  \
          (entry_ptr)->aux_next->aux_prev = (entry_ptr)->aux_prev;           \
       entry_ptr->aux_next = NULL;                                           \
       entry_ptr->aux_prev = NULL;                                           \
       (len)--;                                                              \
       (Size) -= entry_ptr->size;                                            \
    }                                                                        \
} /* H5C__AUX_DLL_REMOVE() */

#if H5C_DO_SANITY_CHECKS

#define H5C__IL_DLL_PRE_REMOVE_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv) \
if ( ( (hd_ptr) == NULL ) ||                                                  \
     ( (tail_ptr) == NULL ) ||                                                \
     ( (entry_ptr) == NULL ) ||                                               \
     ( (len) <= 0 ) ||                                                        \
     ( (Size) < (entry_ptr)->size ) ||                                        \
     ( ( (Size) == (entry_ptr)->size ) && ( ! ( (len) == 1 ) ) ) ||           \
     ( ( (entry_ptr)->il_prev == NULL ) && ( (hd_ptr) != (entry_ptr) ) ) ||   \
     ( ( (entry_ptr)->il_next == NULL ) && ( (tail_ptr) != (entry_ptr) ) ) || \
     ( ( (len) == 1 ) &&                                                      \
       ( ! ( ( (hd_ptr) == (entry_ptr) ) && ( (tail_ptr) == (entry_ptr) ) &&  \
             ( (entry_ptr)->il_next == NULL ) &&                              \
             ( (entry_ptr)->il_prev == NULL ) &&                              \
             ( (Size) == (entry_ptr)->size )                                  \
           )                                                                  \
       )                                                                      \
     )                                                                        \
   ) {                                                                        \
    HDassert(0 && "il DLL pre remove SC failed");                             \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "il DLL pre remove SC failed")   \
}

#define H5C__IL_DLL_PRE_INSERT_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv) \
if ( ( (entry_ptr) == NULL ) ||                                               \
     ( (entry_ptr)->il_next != NULL ) ||                                      \
     ( (entry_ptr)->il_prev != NULL ) ||                                      \
     ( ( ( (hd_ptr) == NULL ) || ( (tail_ptr) == NULL ) ) &&                  \
       ( (hd_ptr) != (tail_ptr) )                                             \
     ) ||                                                                     \
     ( ( (len) == 1 ) &&                                                      \
       ( ( (hd_ptr) != (tail_ptr) ) || ( (Size) <= 0 ) ||                     \
         ( (hd_ptr) == NULL ) || ( (hd_ptr)->size != (Size) )                 \
       )                                                                      \
     ) ||                                                                     \
     ( ( (len) >= 1 ) &&                                                      \
       ( ( (hd_ptr) == NULL ) || ( (hd_ptr)->il_prev != NULL ) ||             \
         ( (tail_ptr) == NULL ) || ( (tail_ptr)->il_next != NULL )            \
       )                                                                      \
     )                                                                        \
   ) {                                                                        \
    HDassert(0 && "IL DLL pre insert SC failed");                             \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "IL DLL pre insert SC failed")   \
}

#define H5C__IL_DLL_SC(head_ptr, tail_ptr, len, Size, fv)                  \
if ( ( ( ( (head_ptr) == NULL ) || ( (tail_ptr) == NULL ) ) &&             \
       ( (head_ptr) != (tail_ptr) )                                        \
     ) ||                                                                  \
     ( ( (len) == 1 ) &&                                                   \
       ( ( (head_ptr) != (tail_ptr) ) ||                                   \
         ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )          \
       )                                                                   \
     ) ||                                                                  \
     ( ( (len) >= 1 ) &&                                                   \
       ( ( (head_ptr) == NULL ) || ( (head_ptr)->il_prev != NULL ) ||      \
         ( (tail_ptr) == NULL ) || ( (tail_ptr)->il_next != NULL )         \
       )                                                                   \
     )                                                                     \
   ) {                                                                     \
    HDassert(0 && "IL DLL sanity check failed");                           \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "IL DLL sanity check failed") \
}

#else /* H5C_DO_SANITY_CHECKS */

#define H5C__IL_DLL_PRE_REMOVE_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv)
#define H5C__IL_DLL_PRE_INSERT_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv)
#define H5C__IL_DLL_SC(head_ptr, tail_ptr, len, Size, fv)

#endif /* H5C_DO_SANITY_CHECKS */


#define H5C__IL_DLL_APPEND(entry_ptr, head_ptr, tail_ptr, len, Size, fail_val)\
{                                                                             \
    H5C__IL_DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size,       \
                               fail_val)                                      \
    if ( (head_ptr) == NULL )                                                 \
    {                                                                         \
       (head_ptr) = (entry_ptr);                                              \
       (tail_ptr) = (entry_ptr);                                              \
    }                                                                         \
    else                                                                      \
    {                                                                         \
       (tail_ptr)->il_next = (entry_ptr);                                     \
       (entry_ptr)->il_prev = (tail_ptr);                                     \
       (tail_ptr) = (entry_ptr);                                              \
    }                                                                         \
    (len)++;                                                                  \
    (Size) += entry_ptr->size;                                                \
    H5C__IL_DLL_SC(head_ptr, tail_ptr, len, Size, fail_val)                   \
} /* H5C__IL_DLL_APPEND() */

#define H5C__IL_DLL_REMOVE(entry_ptr, head_ptr, tail_ptr, len, Size, fv)    \
{                                                                           \
    H5C__IL_DLL_PRE_REMOVE_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv) \
    {                                                                       \
       if ( (head_ptr) == (entry_ptr) )                                     \
       {                                                                    \
          (head_ptr) = (entry_ptr)->il_next;                                \
          if ( (head_ptr) != NULL )                                         \
             (head_ptr)->il_prev = NULL;                                    \
       }                                                                    \
       else                                                                 \
          (entry_ptr)->il_prev->il_next = (entry_ptr)->il_next;             \
       if ( (tail_ptr) == (entry_ptr) )                                     \
       {                                                                    \
          (tail_ptr) = (entry_ptr)->il_prev;                                \
          if ( (tail_ptr) != NULL )                                         \
             (tail_ptr)->il_next = NULL;                                    \
       }                                                                    \
       else                                                                 \
          (entry_ptr)->il_next->il_prev = (entry_ptr)->il_prev;             \
       entry_ptr->il_next = NULL;                                           \
       entry_ptr->il_prev = NULL;                                           \
       (len)--;                                                             \
       (Size) -= entry_ptr->size;                                           \
    }                                                                       \
    H5C__IL_DLL_SC(head_ptr, tail_ptr, len, Size, fv)                       \
} /* H5C__IL_DLL_REMOVE() */


/***********************************************************************
 *
 * Stats collection macros
 *
 * The following macros must handle stats collection when this collection
 * is enabled, and evaluate to the empty string when it is not.
 *
 * The sole exception to this rule is
 * H5C__UPDATE_CACHE_HIT_RATE_STATS(), which is always active as
 * the cache hit rate stats are always collected and available.
 *
 ***********************************************************************/

#define H5C__UPDATE_CACHE_HIT_RATE_STATS(cache_ptr, hit) \
        (cache_ptr->cache_accesses)++;                   \
        if ( hit ) {                                     \
            (cache_ptr->cache_hits)++;                   \
        }                                                \

#if H5C_COLLECT_CACHE_STATS

#define H5C__UPDATE_MAX_INDEX_SIZE_STATS(cache_ptr)                        \
        if ( (cache_ptr)->index_size > (cache_ptr)->max_index_size )       \
            (cache_ptr)->max_index_size = (cache_ptr)->index_size;         \
        if ( (cache_ptr)->clean_index_size >                               \
                (cache_ptr)->max_clean_index_size )                        \
            (cache_ptr)->max_clean_index_size =                            \
                (cache_ptr)->clean_index_size;                             \
        if ( (cache_ptr)->dirty_index_size >                               \
                (cache_ptr)->max_dirty_index_size )                        \
            (cache_ptr)->max_dirty_index_size =                            \
                (cache_ptr)->dirty_index_size;

#define H5C__UPDATE_STATS_FOR_DIRTY_PIN(cache_ptr, entry_ptr) \
	(((cache_ptr)->dirty_pins)[(entry_ptr)->type->id])++;

#define H5C__UPDATE_STATS_FOR_UNPROTECT(cache_ptr)                   \
        if ( (cache_ptr)->slist_len > (cache_ptr)->max_slist_len )   \
	    (cache_ptr)->max_slist_len = (cache_ptr)->slist_len;     \
        if ( (cache_ptr)->slist_size > (cache_ptr)->max_slist_size ) \
	    (cache_ptr)->max_slist_size = (cache_ptr)->slist_size;   \
	if ( (cache_ptr)->pel_len > (cache_ptr)->max_pel_len )       \
	    (cache_ptr)->max_pel_len = (cache_ptr)->pel_len;         \
	if ( (cache_ptr)->pel_size > (cache_ptr)->max_pel_size )     \
	    (cache_ptr)->max_pel_size = (cache_ptr)->pel_size;

#define H5C__UPDATE_STATS_FOR_MOVE(cache_ptr, entry_ptr)               \
	if ( cache_ptr->flush_in_progress )                            \
            ((cache_ptr)->cache_flush_moves[(entry_ptr)->type->id])++; \
        if ( entry_ptr->flush_in_progress )                            \
            ((cache_ptr)->entry_flush_moves[(entry_ptr)->type->id])++; \
	(((cache_ptr)->moves)[(entry_ptr)->type->id])++;               \
        (cache_ptr)->entries_relocated_counter++;

#define H5C__UPDATE_STATS_FOR_ENTRY_SIZE_CHANGE(cache_ptr, entry_ptr, new_size)\
	if ( cache_ptr->flush_in_progress )                                    \
            ((cache_ptr)->cache_flush_size_changes[(entry_ptr)->type->id])++;  \
        if ( entry_ptr->flush_in_progress )                                    \
            ((cache_ptr)->entry_flush_size_changes[(entry_ptr)->type->id])++;  \
	if ( (entry_ptr)->size < (new_size) ) {                                \
	    ((cache_ptr)->size_increases[(entry_ptr)->type->id])++;            \
            H5C__UPDATE_MAX_INDEX_SIZE_STATS(cache_ptr)                        \
            if ( (cache_ptr)->slist_size > (cache_ptr)->max_slist_size )       \
                (cache_ptr)->max_slist_size = (cache_ptr)->slist_size;         \
            if ( (cache_ptr)->pl_size > (cache_ptr)->max_pl_size )             \
                (cache_ptr)->max_pl_size = (cache_ptr)->pl_size;               \
	} else if ( (entry_ptr)->size > (new_size) ) {                         \
	    ((cache_ptr)->size_decreases[(entry_ptr)->type->id])++;            \
	}

#define H5C__UPDATE_STATS_FOR_HT_INSERTION(cache_ptr) \
	(cache_ptr)->total_ht_insertions++;

#define H5C__UPDATE_STATS_FOR_HT_DELETION(cache_ptr) \
	(cache_ptr)->total_ht_deletions++;

#define H5C__UPDATE_STATS_FOR_HT_SEARCH(cache_ptr, success, depth)  \
	if ( success ) {                                            \
	    (cache_ptr)->successful_ht_searches++;                  \
	    (cache_ptr)->total_successful_ht_search_depth += depth; \
	} else {                                                    \
	    (cache_ptr)->failed_ht_searches++;                      \
	    (cache_ptr)->total_failed_ht_search_depth += depth;     \
	}

#define H5C__UPDATE_STATS_FOR_UNPIN(cache_ptr, entry_ptr) \
	((cache_ptr)->unpins)[(entry_ptr)->type->id]++;

#define H5C__UPDATE_STATS_FOR_SLIST_SCAN_RESTART(cache_ptr) \
	((cache_ptr)->slist_scan_restarts)++;

#define H5C__UPDATE_STATS_FOR_LRU_SCAN_RESTART(cache_ptr) \
	((cache_ptr)->LRU_scan_restarts)++;

#define H5C__UPDATE_STATS_FOR_INDEX_SCAN_RESTART(cache_ptr) \
	((cache_ptr)->index_scan_restarts)++;

#define H5C__UPDATE_STATS_FOR_CACHE_IMAGE_CREATE(cache_ptr) \
{                                                           \
    (cache_ptr)->images_created++;                          \
}

#define H5C__UPDATE_STATS_FOR_CACHE_IMAGE_READ(cache_ptr)  \
{                                                          \
    /* make sure image len is still good */                \
    HDassert((cache_ptr)->image_len > 0);                  \
    (cache_ptr)->images_read++;                            \
}

#define H5C__UPDATE_STATS_FOR_CACHE_IMAGE_LOAD(cache_ptr)  \
{                                                          \
    /* make sure image len is still good */                \
    HDassert((cache_ptr)->image_len > 0);                  \
    (cache_ptr)->images_loaded++;                          \
    (cache_ptr)->last_image_size = (cache_ptr)->image_len; \
}

#define H5C__UPDATE_STATS_FOR_PREFETCH(cache_ptr, dirty) \
{                                                        \
    (cache_ptr)->prefetches++;                           \
    if ( dirty )                                         \
        (cache_ptr)->dirty_prefetches++;                 \
}

#define H5C__UPDATE_STATS_FOR_PREFETCH_HIT(cache_ptr) \
{                                                     \
    (cache_ptr)->prefetch_hits++;                     \
}

#if H5C_COLLECT_CACHE_ENTRY_STATS

#define H5C__RESET_CACHE_ENTRY_STATS(entry_ptr) \
{                                           \
    (entry_ptr)->accesses = 0;              \
    (entry_ptr)->clears   = 0;              \
    (entry_ptr)->flushes  = 0;              \
    (entry_ptr)->pins     = 0;              \
}

#define H5C__UPDATE_STATS_FOR_CLEAR(cache_ptr, entry_ptr)        \
{                                                                \
    (((cache_ptr)->clears)[(entry_ptr)->type->id])++;            \
    if((entry_ptr)->is_pinned)                                   \
        (((cache_ptr)->pinned_clears)[(entry_ptr)->type->id])++; \
    ((entry_ptr)->clears)++;                                     \
}

#define H5C__UPDATE_STATS_FOR_FLUSH(cache_ptr, entry_ptr)         \
{                                                                 \
    (((cache_ptr)->flushes)[(entry_ptr)->type->id])++;            \
    if((entry_ptr)->is_pinned)                                    \
        (((cache_ptr)->pinned_flushes)[(entry_ptr)->type->id])++; \
    ((entry_ptr)->flushes)++;                                     \
}

#define H5C__UPDATE_STATS_FOR_EVICTION(cache_ptr, entry_ptr, take_ownership) \
{                                                                            \
    if ( take_ownership )                                                    \
        (((cache_ptr)->take_ownerships)[(entry_ptr)->type->id])++;           \
    else                                                                     \
        (((cache_ptr)->evictions)[(entry_ptr)->type->id])++;                 \
    if ( (entry_ptr)->accesses >                                         \
            ((cache_ptr)->max_accesses)[(entry_ptr)->type->id] )         \
        ((cache_ptr)->max_accesses)[(entry_ptr)->type->id] =             \
            (entry_ptr)->accesses;                                       \
    if ( (entry_ptr)->accesses <                                         \
            ((cache_ptr)->min_accesses)[(entry_ptr)->type->id] )         \
        ((cache_ptr)->min_accesses)[(entry_ptr)->type->id] =             \
            (entry_ptr)->accesses;                                       \
    if ( (entry_ptr)->clears >                                           \
             ((cache_ptr)->max_clears)[(entry_ptr)->type->id] )          \
            ((cache_ptr)->max_clears)[(entry_ptr)->type->id]             \
                 = (entry_ptr)->clears;                                  \
    if ( (entry_ptr)->flushes >                                          \
            ((cache_ptr)->max_flushes)[(entry_ptr)->type->id] )          \
        ((cache_ptr)->max_flushes)[(entry_ptr)->type->id]                \
            = (entry_ptr)->flushes;                                      \
    if ( (entry_ptr)->size >                                             \
            ((cache_ptr)->max_size)[(entry_ptr)->type->id] )             \
        ((cache_ptr)->max_size)[(entry_ptr)->type->id]                   \
            = (entry_ptr)->size;                                         \
    if ( (entry_ptr)->pins >                                             \
            ((cache_ptr)->max_pins)[(entry_ptr)->type->id] )             \
        ((cache_ptr)->max_pins)[(entry_ptr)->type->id]                   \
            = (entry_ptr)->pins;                                         \
}

#define H5C__UPDATE_STATS_FOR_INSERTION(cache_ptr, entry_ptr)        \
{                                                                    \
    (((cache_ptr)->insertions)[(entry_ptr)->type->id])++;            \
    if ( (entry_ptr)->is_pinned ) {                                  \
        (((cache_ptr)->pinned_insertions)[(entry_ptr)->type->id])++; \
        ((cache_ptr)->pins)[(entry_ptr)->type->id]++;                \
        (entry_ptr)->pins++;                                         \
        if ( (cache_ptr)->pel_len > (cache_ptr)->max_pel_len )       \
            (cache_ptr)->max_pel_len = (cache_ptr)->pel_len;         \
        if ( (cache_ptr)->pel_size > (cache_ptr)->max_pel_size )     \
            (cache_ptr)->max_pel_size = (cache_ptr)->pel_size;       \
    }                                                                \
    if ( (cache_ptr)->index_len > (cache_ptr)->max_index_len )       \
        (cache_ptr)->max_index_len = (cache_ptr)->index_len;         \
    H5C__UPDATE_MAX_INDEX_SIZE_STATS(cache_ptr)                      \
    if ( (cache_ptr)->slist_len > (cache_ptr)->max_slist_len )       \
        (cache_ptr)->max_slist_len = (cache_ptr)->slist_len;         \
    if ( (cache_ptr)->slist_size > (cache_ptr)->max_slist_size )     \
        (cache_ptr)->max_slist_size = (cache_ptr)->slist_size;       \
    if ( (entry_ptr)->size >                                         \
            ((cache_ptr)->max_size)[(entry_ptr)->type->id] )         \
        ((cache_ptr)->max_size)[(entry_ptr)->type->id]               \
             = (entry_ptr)->size;                                    \
    cache_ptr->entries_inserted_counter++;                           \
}

#define H5C__UPDATE_STATS_FOR_PROTECT(cache_ptr, entry_ptr, hit)            \
{                                                                           \
    if ( hit )                                                              \
        ((cache_ptr)->hits)[(entry_ptr)->type->id]++;                       \
    else                                                                    \
        ((cache_ptr)->misses)[(entry_ptr)->type->id]++;                     \
    if ( ! ((entry_ptr)->is_read_only) ) {                                  \
        ((cache_ptr)->write_protects)[(entry_ptr)->type->id]++;             \
    } else {                                                                \
        ((cache_ptr)->read_protects)[(entry_ptr)->type->id]++;              \
        if ( ((entry_ptr)->ro_ref_count) >                                  \
                ((cache_ptr)->max_read_protects)[(entry_ptr)->type->id] )   \
            ((cache_ptr)->max_read_protects)[(entry_ptr)->type->id] =       \
                    ((entry_ptr)->ro_ref_count);                            \
    }                                                                       \
    if ( (cache_ptr)->index_len > (cache_ptr)->max_index_len )              \
        (cache_ptr)->max_index_len = (cache_ptr)->index_len;                \
    H5C__UPDATE_MAX_INDEX_SIZE_STATS(cache_ptr)                             \
    if ( (cache_ptr)->pl_len > (cache_ptr)->max_pl_len )                    \
        (cache_ptr)->max_pl_len = (cache_ptr)->pl_len;                      \
    if ( (cache_ptr)->pl_size > (cache_ptr)->max_pl_size )                  \
        (cache_ptr)->max_pl_size = (cache_ptr)->pl_size;                    \
    if ( (entry_ptr)->size >                                                \
            ((cache_ptr)->max_size)[(entry_ptr)->type->id] )                \
        ((cache_ptr)->max_size)[(entry_ptr)->type->id] = (entry_ptr)->size; \
    ((entry_ptr)->accesses)++;                                              \
}

#define H5C__UPDATE_STATS_FOR_PIN(cache_ptr, entry_ptr)      \
{                                                            \
    ((cache_ptr)->pins)[(entry_ptr)->type->id]++;            \
    (entry_ptr)->pins++;                                     \
    if ( (cache_ptr)->pel_len > (cache_ptr)->max_pel_len )   \
        (cache_ptr)->max_pel_len = (cache_ptr)->pel_len;     \
    if ( (cache_ptr)->pel_size > (cache_ptr)->max_pel_size ) \
        (cache_ptr)->max_pel_size = (cache_ptr)->pel_size;   \
}

#else /* H5C_COLLECT_CACHE_ENTRY_STATS */

#define H5C__RESET_CACHE_ENTRY_STATS(entry_ptr)

#define H5C__UPDATE_STATS_FOR_CLEAR(cache_ptr, entry_ptr)         \
{                                                                 \
    (((cache_ptr)->clears)[(entry_ptr)->type->id])++;             \
    if((entry_ptr)->is_pinned)                                    \
        (((cache_ptr)->pinned_clears)[(entry_ptr)->type->id])++;  \
}

#define H5C__UPDATE_STATS_FOR_FLUSH(cache_ptr, entry_ptr)         \
{                                                                 \
    (((cache_ptr)->flushes)[(entry_ptr)->type->id])++;            \
    if ( (entry_ptr)->is_pinned )                                 \
        (((cache_ptr)->pinned_flushes)[(entry_ptr)->type->id])++; \
}

#define H5C__UPDATE_STATS_FOR_EVICTION(cache_ptr, entry_ptr, take_ownership) \
{                                                                            \
    if ( take_ownership )                                                    \
        (((cache_ptr)->take_ownerships)[(entry_ptr)->type->id])++;           \
    else                                                                     \
        (((cache_ptr)->evictions)[(entry_ptr)->type->id])++;                 \
}

#define H5C__UPDATE_STATS_FOR_INSERTION(cache_ptr, entry_ptr)        \
{                                                                    \
    (((cache_ptr)->insertions)[(entry_ptr)->type->id])++;            \
    if ( (entry_ptr)->is_pinned ) {                                  \
        (((cache_ptr)->pinned_insertions)[(entry_ptr)->type->id])++; \
        ((cache_ptr)->pins)[(entry_ptr)->type->id]++;                \
        if ( (cache_ptr)->pel_len > (cache_ptr)->max_pel_len )       \
            (cache_ptr)->max_pel_len = (cache_ptr)->pel_len;         \
        if ( (cache_ptr)->pel_size > (cache_ptr)->max_pel_size )     \
            (cache_ptr)->max_pel_size = (cache_ptr)->pel_size;       \
    }                                                                \
    if ( (cache_ptr)->index_len > (cache_ptr)->max_index_len )       \
        (cache_ptr)->max_index_len = (cache_ptr)->index_len;         \
    H5C__UPDATE_MAX_INDEX_SIZE_STATS(cache_ptr)                      \
    if ( (cache_ptr)->slist_len > (cache_ptr)->max_slist_len )       \
        (cache_ptr)->max_slist_len = (cache_ptr)->slist_len;         \
    if ( (cache_ptr)->slist_size > (cache_ptr)->max_slist_size )     \
        (cache_ptr)->max_slist_size = (cache_ptr)->slist_size;       \
    cache_ptr->entries_inserted_counter++;                           \
}

#define H5C__UPDATE_STATS_FOR_PROTECT(cache_ptr, entry_ptr, hit)            \
{                                                                           \
    if ( hit )                                                              \
        ((cache_ptr)->hits)[(entry_ptr)->type->id]++;                       \
    else                                                                    \
        ((cache_ptr)->misses)[(entry_ptr)->type->id]++;                     \
    if ( ! ((entry_ptr)->is_read_only) )                                    \
        ((cache_ptr)->write_protects)[(entry_ptr)->type->id]++;             \
    else {                                                                  \
        ((cache_ptr)->read_protects)[(entry_ptr)->type->id]++;              \
        if ( ((entry_ptr)->ro_ref_count) >                                  \
                ((cache_ptr)->max_read_protects)[(entry_ptr)->type->id] )   \
            ((cache_ptr)->max_read_protects)[(entry_ptr)->type->id] =       \
                    ((entry_ptr)->ro_ref_count);                            \
    }                                                                       \
    if ( (cache_ptr)->index_len > (cache_ptr)->max_index_len )              \
        (cache_ptr)->max_index_len = (cache_ptr)->index_len;                \
    H5C__UPDATE_MAX_INDEX_SIZE_STATS(cache_ptr)                             \
    if ( (cache_ptr)->pl_len > (cache_ptr)->max_pl_len )                    \
        (cache_ptr)->max_pl_len = (cache_ptr)->pl_len;                      \
    if ( (cache_ptr)->pl_size > (cache_ptr)->max_pl_size )                  \
        (cache_ptr)->max_pl_size = (cache_ptr)->pl_size;                    \
}

#define H5C__UPDATE_STATS_FOR_PIN(cache_ptr, entry_ptr)      \
{                                                            \
    ((cache_ptr)->pins)[(entry_ptr)->type->id]++;            \
    if ( (cache_ptr)->pel_len > (cache_ptr)->max_pel_len )   \
        (cache_ptr)->max_pel_len = (cache_ptr)->pel_len;     \
    if ( (cache_ptr)->pel_size > (cache_ptr)->max_pel_size ) \
        (cache_ptr)->max_pel_size = (cache_ptr)->pel_size;   \
}

#endif /* H5C_COLLECT_CACHE_ENTRY_STATS */

#else /* H5C_COLLECT_CACHE_STATS */

#define H5C__RESET_CACHE_ENTRY_STATS(entry_ptr)
#define H5C__UPDATE_STATS_FOR_DIRTY_PIN(cache_ptr, entry_ptr)
#define H5C__UPDATE_STATS_FOR_UNPROTECT(cache_ptr)
#define H5C__UPDATE_STATS_FOR_MOVE(cache_ptr, entry_ptr)
#define H5C__UPDATE_STATS_FOR_ENTRY_SIZE_CHANGE(cache_ptr, entry_ptr, new_size)
#define H5C__UPDATE_STATS_FOR_HT_INSERTION(cache_ptr)
#define H5C__UPDATE_STATS_FOR_HT_DELETION(cache_ptr)
#define H5C__UPDATE_STATS_FOR_HT_SEARCH(cache_ptr, success, depth)
#define H5C__UPDATE_STATS_FOR_INSERTION(cache_ptr, entry_ptr)
#define H5C__UPDATE_STATS_FOR_CLEAR(cache_ptr, entry_ptr)
#define H5C__UPDATE_STATS_FOR_FLUSH(cache_ptr, entry_ptr)
#define H5C__UPDATE_STATS_FOR_EVICTION(cache_ptr, entry_ptr, take_ownership)
#define H5C__UPDATE_STATS_FOR_PROTECT(cache_ptr, entry_ptr, hit)
#define H5C__UPDATE_STATS_FOR_PIN(cache_ptr, entry_ptr)
#define H5C__UPDATE_STATS_FOR_UNPIN(cache_ptr, entry_ptr)
#define H5C__UPDATE_STATS_FOR_SLIST_SCAN_RESTART(cache_ptr)
#define H5C__UPDATE_STATS_FOR_LRU_SCAN_RESTART(cache_ptr)
#define H5C__UPDATE_STATS_FOR_INDEX_SCAN_RESTART(cache_ptr)
#define H5C__UPDATE_STATS_FOR_CACHE_IMAGE_CREATE(cache_ptr)
#define H5C__UPDATE_STATS_FOR_CACHE_IMAGE_READ(cache_ptr)
#define H5C__UPDATE_STATS_FOR_CACHE_IMAGE_LOAD(cache_ptr)
#define H5C__UPDATE_STATS_FOR_PREFETCH(cache_ptr, dirty)
#define H5C__UPDATE_STATS_FOR_PREFETCH_HIT(cache_ptr)

#endif /* H5C_COLLECT_CACHE_STATS */


/***********************************************************************
 *
 * Hash table access and manipulation macros:
 *
 * The following macros handle searches, insertions, and deletion in
 * the hash table.
 *
 * When modifying these macros, remember to modify the similar macros
 * in tst/cache.c
 *
 * Changes:
 *
 *   - Updated existing index macros and sanity check macros to maintain
 *     the clean_index_size and dirty_index_size fields of H5C_t.  Also
 *     added macros to allow us to track entry cleans and dirties.
 *
 *     						JRM -- 11/5/08
 *
 *   - Updated existing index macros and sanity check macros to maintain 
 *     the index_ring_len, index_ring_size, clean_index_ring_size, and
 *     dirty_index_ring_size fields of H5C_t.
 *
 *						JRM -- 9/1/15
 *
 *   - Updated existing index macros and sanity checks macros to 
 *     maintain an doubly linked list of all entries in the index.
 *     This is necessary to reduce the computational cost of visiting
 *     all entries in the index, which used to be done by scanning 
 *     the hash table.
 *
 *                                              JRM -- 10/15/15
 *
 ***********************************************************************/

/* H5C__HASH_TABLE_LEN is defined in H5Cpkg.h.  It mut be a power of two. */

#define H5C__HASH_MASK		((size_t)(H5C__HASH_TABLE_LEN - 1) << 3)

#define H5C__HASH_FCN(x)	(int)((unsigned)((x) & H5C__HASH_MASK) >> 3)

#if H5C_DO_SANITY_CHECKS

#define H5C__PRE_HT_INSERT_SC(cache_ptr, entry_ptr, fail_val)           \
if ( ( (cache_ptr) == NULL ) ||                                         \
     ( (cache_ptr)->magic != H5C__H5C_T_MAGIC ) ||                      \
     ( (entry_ptr) == NULL ) ||                                         \
     ( ! H5F_addr_defined((entry_ptr)->addr) ) ||                       \
     ( (entry_ptr)->ht_next != NULL ) ||                                \
     ( (entry_ptr)->ht_prev != NULL ) ||                                \
     ( (entry_ptr)->size <= 0 ) ||                                      \
     ( H5C__HASH_FCN((entry_ptr)->addr) < 0 ) ||                        \
     ( H5C__HASH_FCN((entry_ptr)->addr) >= H5C__HASH_TABLE_LEN ) ||     \
     ( (cache_ptr)->index_size !=                                       \
       ((cache_ptr)->clean_index_size +                                 \
	(cache_ptr)->dirty_index_size) ) ||                             \
     ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||   \
     ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||   \
     ( (entry_ptr)->ring <= H5C_RING_UNDEFINED ) ||                     \
     ( (entry_ptr)->ring >= H5C_RING_NTYPES ) ||                        \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                 \
       (cache_ptr)->index_len ) ||                                      \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                \
       (cache_ptr)->index_size ) ||                                     \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=               \
       ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +         \
        (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ||     \
     ( (cache_ptr)->index_len != (cache_ptr)->il_len ) ||               \
     ( (cache_ptr)->index_size != (cache_ptr)->il_size ) ) {            \
    HDassert(FALSE);                                                    \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, fail_val, "pre HT insert SC failed") \
}

#define H5C__POST_HT_INSERT_SC(cache_ptr, entry_ptr, fail_val)          \
if ( ( (cache_ptr) == NULL ) ||                                         \
     ( (cache_ptr)->magic != H5C__H5C_T_MAGIC ) ||                      \
     ( (cache_ptr)->index_size !=                                       \
       ((cache_ptr)->clean_index_size +                                 \
	(cache_ptr)->dirty_index_size) ) ||                             \
     ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||   \
     ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||   \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] == 0 ) ||         \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                 \
       (cache_ptr)->index_len ) ||                                      \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                \
       (cache_ptr)->index_size ) ||                                     \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=               \
       ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +         \
        (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ||     \
     ( (cache_ptr)->index_len != (cache_ptr)->il_len ) ||               \
     ( (cache_ptr)->index_size != (cache_ptr)->il_size) ) {             \
    HDassert(FALSE);                                                    \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, fail_val, "post HT insert SC failed") \
}

#define H5C__PRE_HT_REMOVE_SC(cache_ptr, entry_ptr)                     \
if ( ( (cache_ptr) == NULL ) ||                                         \
     ( (cache_ptr)->magic != H5C__H5C_T_MAGIC ) ||                      \
     ( (cache_ptr)->index_len < 1 ) ||                                  \
     ( (entry_ptr) == NULL ) ||                                         \
     ( (cache_ptr)->index_size < (entry_ptr)->size ) ||                 \
     ( ! H5F_addr_defined((entry_ptr)->addr) ) ||                       \
     ( (entry_ptr)->size <= 0 ) ||                                      \
     ( H5C__HASH_FCN((entry_ptr)->addr) < 0 ) ||                        \
     ( H5C__HASH_FCN((entry_ptr)->addr) >= H5C__HASH_TABLE_LEN ) ||     \
     ( ((cache_ptr)->index)[(H5C__HASH_FCN((entry_ptr)->addr))]         \
       == NULL ) ||                                                     \
     ( ( ((cache_ptr)->index)[(H5C__HASH_FCN((entry_ptr)->addr))]       \
       != (entry_ptr) ) &&                                              \
       ( (entry_ptr)->ht_prev == NULL ) ) ||                            \
     ( ( ((cache_ptr)->index)[(H5C__HASH_FCN((entry_ptr)->addr))] ==    \
         (entry_ptr) ) &&                                               \
       ( (entry_ptr)->ht_prev != NULL ) ) ||                            \
     ( (cache_ptr)->index_size !=                                       \
       ((cache_ptr)->clean_index_size +                                 \
	(cache_ptr)->dirty_index_size) ) ||                             \
     ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||   \
     ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||   \
     ( (entry_ptr)->ring <= H5C_RING_UNDEFINED ) ||                     \
     ( (entry_ptr)->ring >= H5C_RING_NTYPES ) ||                        \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] <= 0 ) ||         \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                 \
       (cache_ptr)->index_len ) ||                                      \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] <                \
       (entry_ptr)->size ) ||                                           \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                \
       (cache_ptr)->index_size ) ||                                     \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=               \
       ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +         \
        (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ||     \
     ( (cache_ptr)->index_len != (cache_ptr)->il_len ) ||               \
     ( (cache_ptr)->index_size != (cache_ptr)->il_size ) ) {            \
    HDassert(FALSE);                                                    \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "pre HT remove SC failed") \
}

#define H5C__POST_HT_REMOVE_SC(cache_ptr, entry_ptr)                     \
if ( ( (cache_ptr) == NULL ) ||                                          \
     ( (cache_ptr)->magic != H5C__H5C_T_MAGIC ) ||                       \
     ( (entry_ptr) == NULL ) ||                                          \
     ( ! H5F_addr_defined((entry_ptr)->addr) ) ||                        \
     ( (entry_ptr)->size <= 0 ) ||                                       \
     ( (entry_ptr)->ht_prev != NULL ) ||                                 \
     ( (entry_ptr)->ht_prev != NULL ) ||                                 \
     ( (cache_ptr)->index_size !=                                        \
       ((cache_ptr)->clean_index_size +                                  \
	(cache_ptr)->dirty_index_size) ) ||                              \
     ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||    \
     ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||    \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                  \
       (cache_ptr)->index_len ) ||                                       \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                 \
       (cache_ptr)->index_size ) ||                                      \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=                \
       ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +          \
        (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ||      \
     ( (cache_ptr)->index_len != (cache_ptr)->il_len ) ||                \
     ( (cache_ptr)->index_size != (cache_ptr)->il_size ) ) {             \
    HDassert(FALSE);                                                     \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "post HT remove SC failed") \
}

/* (Keep in sync w/H5C_TEST__PRE_HT_SEARCH_SC macro in test/cache_common.h -QAK) */
#define H5C__PRE_HT_SEARCH_SC(cache_ptr, Addr, fail_val)                    \
if ( ( (cache_ptr) == NULL ) ||                                             \
     ( (cache_ptr)->magic != H5C__H5C_T_MAGIC ) ||                          \
     ( (cache_ptr)->index_size !=                                           \
       ((cache_ptr)->clean_index_size + (cache_ptr)->dirty_index_size) ) || \
     ( ! H5F_addr_defined(Addr) ) ||                                        \
     ( H5C__HASH_FCN(Addr) < 0 ) ||                                         \
     ( H5C__HASH_FCN(Addr) >= H5C__HASH_TABLE_LEN ) ) {                     \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, fail_val, "pre HT search SC failed") \
}

/* (Keep in sync w/H5C_TEST__POST_SUC_HT_SEARCH_SC macro in test/cache_common.h -QAK) */
#define H5C__POST_SUC_HT_SEARCH_SC(cache_ptr, entry_ptr, k, fail_val)       \
if ( ( (cache_ptr) == NULL ) ||                                             \
     ( (cache_ptr)->magic != H5C__H5C_T_MAGIC ) ||                          \
     ( (cache_ptr)->index_len < 1 ) ||                                      \
     ( (entry_ptr) == NULL ) ||                                             \
     ( (cache_ptr)->index_size < (entry_ptr)->size ) ||                     \
     ( (cache_ptr)->index_size !=                                           \
       ((cache_ptr)->clean_index_size + (cache_ptr)->dirty_index_size) ) || \
     ( (entry_ptr)->size <= 0 ) ||                                          \
     ( ((cache_ptr)->index)[k] == NULL ) ||                                 \
     ( ( ((cache_ptr)->index)[k] != (entry_ptr) ) &&                        \
       ( (entry_ptr)->ht_prev == NULL ) ) ||                                \
     ( ( ((cache_ptr)->index)[k] == (entry_ptr) ) &&                        \
       ( (entry_ptr)->ht_prev != NULL ) ) ||                                \
     ( ( (entry_ptr)->ht_prev != NULL ) &&                                  \
       ( (entry_ptr)->ht_prev->ht_next != (entry_ptr) ) ) ||                \
     ( ( (entry_ptr)->ht_next != NULL ) &&                                  \
       ( (entry_ptr)->ht_next->ht_prev != (entry_ptr) ) ) ) {               \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, fail_val, "post successful HT search SC failed") \
}

/* (Keep in sync w/H5C_TEST__POST_HT_SHIFT_TO_FRONT macro in test/cache_common.h -QAK) */
#define H5C__POST_HT_SHIFT_TO_FRONT(cache_ptr, entry_ptr, k, fail_val) \
if ( ( (cache_ptr) == NULL ) ||                                        \
     ( ((cache_ptr)->index)[k] != (entry_ptr) ) ||                     \
     ( (entry_ptr)->ht_prev != NULL ) ) {                              \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, fail_val, "post HT shift to front SC failed") \
}

#define H5C__PRE_HT_ENTRY_SIZE_CHANGE_SC(cache_ptr, old_size, new_size, \
		                         entry_ptr, was_clean)          \
if ( ( (cache_ptr) == NULL ) ||                                         \
     ( (cache_ptr)->index_len <= 0 ) ||                                 \
     ( (cache_ptr)->index_size <= 0 ) ||                                \
     ( (new_size) <= 0 ) ||                                             \
     ( (old_size) > (cache_ptr)->index_size ) ||                        \
     ( ( (cache_ptr)->index_len == 1 ) &&                               \
       ( (cache_ptr)->index_size != (old_size) ) ) ||                   \
     ( (cache_ptr)->index_size !=                                       \
       ((cache_ptr)->clean_index_size +                                 \
        (cache_ptr)->dirty_index_size) ) ||                             \
     ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||   \
     ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||   \
     ( ( !( was_clean ) ||                                              \
	    ( (cache_ptr)->clean_index_size < (old_size) ) ) &&         \
	  ( ( (was_clean) ) ||                                          \
	    ( (cache_ptr)->dirty_index_size < (old_size) ) ) ) ||       \
     ( (entry_ptr) == NULL ) ||                                         \
     ( (entry_ptr)->ring <= H5C_RING_UNDEFINED ) ||                     \
     ( (entry_ptr)->ring >= H5C_RING_NTYPES ) ||                        \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] <= 0 ) ||         \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                 \
       (cache_ptr)->index_len ) ||                                      \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                \
       (cache_ptr)->index_size ) ||                                     \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=               \
       ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +         \
        (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ||     \
     ( (cache_ptr)->index_len != (cache_ptr)->il_len ) ||               \
     ( (cache_ptr)->index_size != (cache_ptr)->il_size ) ) {            \
    HDassert(FALSE);                                                    \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "pre HT entry size change SC failed") \
}

#define H5C__POST_HT_ENTRY_SIZE_CHANGE_SC(cache_ptr, old_size, new_size,  \
		                          entry_ptr)                      \
if ( ( (cache_ptr) == NULL ) ||                                           \
     ( (cache_ptr)->index_len <= 0 ) ||                                   \
     ( (cache_ptr)->index_size <= 0 ) ||                                  \
     ( (new_size) > (cache_ptr)->index_size ) ||                          \
     ( (cache_ptr)->index_size !=                                         \
	  ((cache_ptr)->clean_index_size +                                \
           (cache_ptr)->dirty_index_size) ) ||                            \
     ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||     \
     ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||     \
     ( ( !((entry_ptr)->is_dirty ) ||                                     \
	    ( (cache_ptr)->dirty_index_size < (new_size) ) ) &&           \
	  ( ( ((entry_ptr)->is_dirty)  ) ||                               \
	    ( (cache_ptr)->clean_index_size < (new_size) ) ) ) ||         \
     ( ( (cache_ptr)->index_len == 1 ) &&                                 \
       ( (cache_ptr)->index_size != (new_size) ) ) ||                     \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                   \
       (cache_ptr)->index_len ) ||                                        \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                  \
       (cache_ptr)->index_size ) ||                                       \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=                 \
       ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +           \
        (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ||       \
     ( (cache_ptr)->index_len != (cache_ptr)->il_len ) ||                 \
     ( (cache_ptr)->index_size != (cache_ptr)->il_size ) ) {              \
    HDassert(FALSE);                                                      \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "post HT entry size change SC failed") \
}

#define H5C__PRE_HT_UPDATE_FOR_ENTRY_CLEAN_SC(cache_ptr, entry_ptr)           \
if (                                                                          \
    ( (cache_ptr) == NULL ) ||                                                \
    ( (cache_ptr)->magic != H5C__H5C_T_MAGIC ) ||                             \
    ( (cache_ptr)->index_len <= 0 ) ||                                        \
    ( (entry_ptr) == NULL ) ||                                                \
    ( (entry_ptr)->is_dirty != FALSE ) ||                                     \
    ( (cache_ptr)->index_size < (entry_ptr)->size ) ||                        \
    ( (cache_ptr)->dirty_index_size < (entry_ptr)->size ) ||                  \
    ( (cache_ptr)->index_size !=                                              \
       ((cache_ptr)->clean_index_size + (cache_ptr)->dirty_index_size) ) ||   \
    ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||          \
    ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||          \
    ( (entry_ptr)->ring <= H5C_RING_UNDEFINED ) ||                            \
    ( (entry_ptr)->ring >= H5C_RING_NTYPES ) ||                               \
    ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] <= 0 ) ||                \
    ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                        \
      (cache_ptr)->index_len ) ||                                             \
    ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                       \
      (cache_ptr)->index_size ) ||                                            \
    ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=                      \
      ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +                \
       (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ) {           \
    HDassert(FALSE);                                                          \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "pre HT update for entry clean SC failed") \
}

#define H5C__PRE_HT_UPDATE_FOR_ENTRY_DIRTY_SC(cache_ptr, entry_ptr)           \
if (                                                                          \
    ( (cache_ptr) == NULL ) ||                                                \
    ( (cache_ptr)->magic != H5C__H5C_T_MAGIC ) ||                             \
    ( (cache_ptr)->index_len <= 0 ) ||                                        \
    ( (entry_ptr) == NULL ) ||                                                \
    ( (entry_ptr)->is_dirty != TRUE ) ||                                      \
    ( (cache_ptr)->index_size < (entry_ptr)->size ) ||                        \
    ( (cache_ptr)->clean_index_size < (entry_ptr)->size ) ||                  \
    ( (cache_ptr)->index_size !=                                              \
       ((cache_ptr)->clean_index_size + (cache_ptr)->dirty_index_size) ) ||   \
    ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||          \
    ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||          \
    ( (entry_ptr)->ring <= H5C_RING_UNDEFINED ) ||                            \
    ( (entry_ptr)->ring >= H5C_RING_NTYPES ) ||                               \
    ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] <= 0 ) ||                \
    ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                        \
      (cache_ptr)->index_len ) ||                                             \
    ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                       \
      (cache_ptr)->index_size ) ||                                            \
    ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=                      \
      ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +                \
       (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ) {           \
    HDassert(FALSE);                                                          \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "pre HT update for entry dirty SC failed") \
}

#define H5C__POST_HT_UPDATE_FOR_ENTRY_CLEAN_SC(cache_ptr, entry_ptr)        \
if ( ( (cache_ptr)->index_size !=                                           \
       ((cache_ptr)->clean_index_size + (cache_ptr)->dirty_index_size) ) || \
     ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||       \
     ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||       \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                     \
       (cache_ptr)->index_len ) ||                                          \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                    \
       (cache_ptr)->index_size ) ||                                         \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=                   \
       ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +             \
        (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ) {        \
    HDassert(FALSE);                                                        \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "post HT update for entry clean SC failed") \
}

#define H5C__POST_HT_UPDATE_FOR_ENTRY_DIRTY_SC(cache_ptr, entry_ptr)        \
if ( ( (cache_ptr)->index_size !=                                           \
       ((cache_ptr)->clean_index_size + (cache_ptr)->dirty_index_size) ) || \
     ( (cache_ptr)->index_size < ((cache_ptr)->clean_index_size) ) ||       \
     ( (cache_ptr)->index_size < ((cache_ptr)->dirty_index_size) ) ||       \
     ( (cache_ptr)->index_ring_len[(entry_ptr)->ring] >                     \
       (cache_ptr)->index_len ) ||                                          \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] >                    \
       (cache_ptr)->index_size ) ||                                         \
     ( (cache_ptr)->index_ring_size[(entry_ptr)->ring] !=                   \
       ((cache_ptr)->clean_index_ring_size[(entry_ptr)->ring] +             \
        (cache_ptr)->dirty_index_ring_size[(entry_ptr)->ring]) ) ) {        \
    HDassert(FALSE);                                                        \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "post HT update for entry dirty SC failed") \
}

#else /* H5C_DO_SANITY_CHECKS */

#define H5C__PRE_HT_INSERT_SC(cache_ptr, entry_ptr, fail_val)
#define H5C__POST_HT_INSERT_SC(cache_ptr, entry_ptr, fail_val)
#define H5C__PRE_HT_REMOVE_SC(cache_ptr, entry_ptr)
#define H5C__POST_HT_REMOVE_SC(cache_ptr, entry_ptr)
#define H5C__PRE_HT_SEARCH_SC(cache_ptr, Addr, fail_val)
#define H5C__POST_SUC_HT_SEARCH_SC(cache_ptr, entry_ptr, k, fail_val)
#define H5C__POST_HT_SHIFT_TO_FRONT(cache_ptr, entry_ptr, k, fail_val)
#define H5C__PRE_HT_UPDATE_FOR_ENTRY_CLEAN_SC(cache_ptr, entry_ptr)
#define H5C__PRE_HT_UPDATE_FOR_ENTRY_DIRTY_SC(cache_ptr, entry_ptr)
#define H5C__PRE_HT_ENTRY_SIZE_CHANGE_SC(cache_ptr, old_size, new_size, \
		                         entry_ptr, was_clean)
#define H5C__POST_HT_ENTRY_SIZE_CHANGE_SC(cache_ptr, old_size, new_size, \
		                          entry_ptr)
#define H5C__POST_HT_UPDATE_FOR_ENTRY_CLEAN_SC(cache_ptr, entry_ptr)
#define H5C__POST_HT_UPDATE_FOR_ENTRY_DIRTY_SC(cache_ptr, entry_ptr)

#endif /* H5C_DO_SANITY_CHECKS */


#define H5C__INSERT_IN_INDEX(cache_ptr, entry_ptr, fail_val)                 \
{                                                                            \
    int k;                                                                   \
    H5C__PRE_HT_INSERT_SC(cache_ptr, entry_ptr, fail_val)                    \
    k = H5C__HASH_FCN((entry_ptr)->addr);                                    \
    if(((cache_ptr)->index)[k] != NULL) {                                    \
        (entry_ptr)->ht_next = ((cache_ptr)->index)[k];                      \
        (entry_ptr)->ht_next->ht_prev = (entry_ptr);                         \
    }                                                                        \
    ((cache_ptr)->index)[k] = (entry_ptr);                                   \
    (cache_ptr)->index_len++;                                                \
    (cache_ptr)->index_size += (entry_ptr)->size;                            \
    ((cache_ptr)->index_ring_len[entry_ptr->ring])++;                        \
    ((cache_ptr)->index_ring_size[entry_ptr->ring])                          \
            += (entry_ptr)->size;                                            \
    if((entry_ptr)->is_dirty) {                                              \
        (cache_ptr)->dirty_index_size += (entry_ptr)->size;                  \
        ((cache_ptr)->dirty_index_ring_size[entry_ptr->ring])                \
                += (entry_ptr)->size;                                        \
    } else {                                                                 \
        (cache_ptr)->clean_index_size += (entry_ptr)->size;                  \
        ((cache_ptr)->clean_index_ring_size[entry_ptr->ring])                \
                += (entry_ptr)->size;                                        \
    }                                                                        \
    if((entry_ptr)->flush_me_last) {                                         \
        (cache_ptr)->num_last_entries++;                                     \
        HDassert((cache_ptr)->num_last_entries <= 2);                        \
    }                                                                        \
    H5C__IL_DLL_APPEND((entry_ptr), (cache_ptr)->il_head,                    \
                       (cache_ptr)->il_tail, (cache_ptr)->il_len,            \
                       (cache_ptr)->il_size, fail_val)                       \
    H5C__UPDATE_STATS_FOR_HT_INSERTION(cache_ptr)                            \
    H5C__POST_HT_INSERT_SC(cache_ptr, entry_ptr, fail_val)                   \
}

#define H5C__DELETE_FROM_INDEX(cache_ptr, entry_ptr, fail_val)               \
{                                                                            \
    int k;                                                                   \
    H5C__PRE_HT_REMOVE_SC(cache_ptr, entry_ptr)                              \
    k = H5C__HASH_FCN((entry_ptr)->addr);                                    \
    if((entry_ptr)->ht_next)                                                 \
        (entry_ptr)->ht_next->ht_prev = (entry_ptr)->ht_prev;                \
    if((entry_ptr)->ht_prev)                                                 \
        (entry_ptr)->ht_prev->ht_next = (entry_ptr)->ht_next;                \
    if(((cache_ptr)->index)[k] == (entry_ptr))                               \
        ((cache_ptr)->index)[k] = (entry_ptr)->ht_next;                      \
    (entry_ptr)->ht_next = NULL;                                             \
    (entry_ptr)->ht_prev = NULL;                                             \
    (cache_ptr)->index_len--;                                                \
    (cache_ptr)->index_size -= (entry_ptr)->size;                            \
    ((cache_ptr)->index_ring_len[entry_ptr->ring])--;                        \
    ((cache_ptr)->index_ring_size[entry_ptr->ring])                          \
            -= (entry_ptr)->size;                                            \
    if((entry_ptr)->is_dirty) {                                              \
        (cache_ptr)->dirty_index_size -= (entry_ptr)->size;                  \
        ((cache_ptr)->dirty_index_ring_size[entry_ptr->ring])                \
                -= (entry_ptr)->size;                                        \
    } else {                                                                 \
        (cache_ptr)->clean_index_size -= (entry_ptr)->size;                  \
        ((cache_ptr)->clean_index_ring_size[entry_ptr->ring])                \
                -= (entry_ptr)->size;                                        \
    }                                                                        \
    if((entry_ptr)->flush_me_last) {                                         \
        (cache_ptr)->num_last_entries--;                                     \
        HDassert((cache_ptr)->num_last_entries <= 1);                        \
    }                                                                        \
    H5C__IL_DLL_REMOVE((entry_ptr), (cache_ptr)->il_head,                    \
                       (cache_ptr)->il_tail, (cache_ptr)->il_len,            \
                       (cache_ptr)->il_size, fail_val)                       \
    H5C__UPDATE_STATS_FOR_HT_DELETION(cache_ptr)                             \
    H5C__POST_HT_REMOVE_SC(cache_ptr, entry_ptr)                             \
}

#define H5C__SEARCH_INDEX(cache_ptr, Addr, entry_ptr, fail_val)             \
{                                                                           \
    int k;                                                                  \
    int depth = 0;                                                          \
    H5C__PRE_HT_SEARCH_SC(cache_ptr, Addr, fail_val)                        \
    k = H5C__HASH_FCN(Addr);                                                \
    entry_ptr = ((cache_ptr)->index)[k];                                    \
    while(entry_ptr) {                                                      \
        if(H5F_addr_eq(Addr, (entry_ptr)->addr)) {                          \
            H5C__POST_SUC_HT_SEARCH_SC(cache_ptr, entry_ptr, k, fail_val)   \
            if(entry_ptr != ((cache_ptr)->index)[k]) {                      \
                if((entry_ptr)->ht_next)                                    \
                    (entry_ptr)->ht_next->ht_prev = (entry_ptr)->ht_prev;   \
                HDassert((entry_ptr)->ht_prev != NULL);                     \
                (entry_ptr)->ht_prev->ht_next = (entry_ptr)->ht_next;       \
                ((cache_ptr)->index)[k]->ht_prev = (entry_ptr);             \
                (entry_ptr)->ht_next = ((cache_ptr)->index)[k];             \
                (entry_ptr)->ht_prev = NULL;                                \
                ((cache_ptr)->index)[k] = (entry_ptr);                      \
                H5C__POST_HT_SHIFT_TO_FRONT(cache_ptr, entry_ptr, k, fail_val) \
            }                                                               \
            break;                                                          \
        }                                                                   \
        (entry_ptr) = (entry_ptr)->ht_next;                                 \
        (depth)++;                                                          \
    }                                                                       \
    H5C__UPDATE_STATS_FOR_HT_SEARCH(cache_ptr, (entry_ptr != NULL), depth)  \
}

#define H5C__SEARCH_INDEX_NO_STATS(cache_ptr, Addr, entry_ptr, fail_val)    \
{                                                                           \
    int k;                                                                  \
    H5C__PRE_HT_SEARCH_SC(cache_ptr, Addr, fail_val)                        \
    k = H5C__HASH_FCN(Addr);                                                \
    entry_ptr = ((cache_ptr)->index)[k];                                    \
    while(entry_ptr) {                                                      \
        if(H5F_addr_eq(Addr, (entry_ptr)->addr)) {                          \
            H5C__POST_SUC_HT_SEARCH_SC(cache_ptr, entry_ptr, k, fail_val)   \
            if(entry_ptr != ((cache_ptr)->index)[k]) {                      \
                if((entry_ptr)->ht_next)                                    \
                    (entry_ptr)->ht_next->ht_prev = (entry_ptr)->ht_prev;   \
                HDassert((entry_ptr)->ht_prev != NULL);                     \
                (entry_ptr)->ht_prev->ht_next = (entry_ptr)->ht_next;       \
                ((cache_ptr)->index)[k]->ht_prev = (entry_ptr);             \
                (entry_ptr)->ht_next = ((cache_ptr)->index)[k];             \
                (entry_ptr)->ht_prev = NULL;                                \
                ((cache_ptr)->index)[k] = (entry_ptr);                      \
                H5C__POST_HT_SHIFT_TO_FRONT(cache_ptr, entry_ptr, k, fail_val) \
            }                                                               \
            break;                                                          \
        }                                                                   \
        (entry_ptr) = (entry_ptr)->ht_next;                                 \
    }                                                                       \
}

#define H5C__UPDATE_INDEX_FOR_ENTRY_CLEAN(cache_ptr, entry_ptr)   \
{                                                                 \
    H5C__PRE_HT_UPDATE_FOR_ENTRY_CLEAN_SC(cache_ptr, entry_ptr);  \
    (cache_ptr)->dirty_index_size -= (entry_ptr)->size;           \
    ((cache_ptr)->dirty_index_ring_size[entry_ptr->ring])         \
		-= (entry_ptr)->size;                             \
    (cache_ptr)->clean_index_size += (entry_ptr)->size;           \
    ((cache_ptr)->clean_index_ring_size[entry_ptr->ring])         \
		+= (entry_ptr)->size;                             \
    H5C__POST_HT_UPDATE_FOR_ENTRY_CLEAN_SC(cache_ptr, entry_ptr); \
}

#define H5C__UPDATE_INDEX_FOR_ENTRY_DIRTY(cache_ptr, entry_ptr)   \
{                                                                 \
    H5C__PRE_HT_UPDATE_FOR_ENTRY_DIRTY_SC(cache_ptr, entry_ptr);  \
    (cache_ptr)->clean_index_size -= (entry_ptr)->size;           \
    ((cache_ptr)->clean_index_ring_size[entry_ptr->ring])         \
		-= (entry_ptr)->size;                             \
    (cache_ptr)->dirty_index_size += (entry_ptr)->size;           \
    ((cache_ptr)->dirty_index_ring_size[entry_ptr->ring])         \
		+= (entry_ptr)->size;                             \
    H5C__POST_HT_UPDATE_FOR_ENTRY_DIRTY_SC(cache_ptr, entry_ptr); \
}

#define H5C__UPDATE_INDEX_FOR_SIZE_CHANGE(cache_ptr, old_size, new_size,    \
		                          entry_ptr, was_clean)             \
{                                                                           \
    H5C__PRE_HT_ENTRY_SIZE_CHANGE_SC(cache_ptr, old_size, new_size,         \
		                     entry_ptr, was_clean)                  \
    (cache_ptr)->index_size -= (old_size);                                  \
    (cache_ptr)->index_size += (new_size);                                  \
    ((cache_ptr)->index_ring_size[entry_ptr->ring]) -= (old_size);          \
    ((cache_ptr)->index_ring_size[entry_ptr->ring]) += (new_size);          \
    if(was_clean) {                                                         \
        (cache_ptr)->clean_index_size -= (old_size);                        \
        ((cache_ptr)->clean_index_ring_size[entry_ptr->ring])-= (old_size); \
    } else {                                                                \
	(cache_ptr)->dirty_index_size -= (old_size);                        \
        ((cache_ptr)->dirty_index_ring_size[entry_ptr->ring])-= (old_size); \
    }                                                                       \
    if((entry_ptr)->is_dirty) {                                             \
        (cache_ptr)->dirty_index_size += (new_size);                        \
        ((cache_ptr)->dirty_index_ring_size[entry_ptr->ring])+= (new_size); \
    } else {                                                                \
	(cache_ptr)->clean_index_size += (new_size);                        \
        ((cache_ptr)->clean_index_ring_size[entry_ptr->ring])+= (new_size); \
    }                                                                       \
    H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr)->il_len,                    \
                                    (cache_ptr)->il_size,                   \
                                    (old_size), (new_size))                 \
    H5C__POST_HT_ENTRY_SIZE_CHANGE_SC(cache_ptr, old_size, new_size,        \
                                      entry_ptr)                            \
}


/**************************************************************************
 *
 * Skip list insertion and deletion macros:
 *
 * These used to be functions, but I converted them to macros to avoid some
 * function call overhead.
 *
 **************************************************************************/

/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__INSERT_ENTRY_IN_SLIST
 *
 * Purpose:     Insert the specified instance of H5C_cache_entry_t into
 *		the skip list in the specified instance of H5C_t.  Update
 *		the associated length and size fields.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 5/10/04
 *
 * Modifications:
 *
 *		JRM -- 7/21/04
 *		Updated function to set the in_tree flag when inserting
 *		an entry into the tree.  Also modified the function to
 *		update the tree size and len fields instead of the similar
 *		index fields.
 *
 *		All of this is part of the modifications to support the
 *		hash table.
 *
 *		JRM -- 7/27/04
 *		Converted the function H5C_insert_entry_in_tree() into
 *		the macro H5C__INSERT_ENTRY_IN_TREE in the hopes of
 *		wringing a little more speed out of the cache.
 *
 *		Note that we don't bother to check if the entry is already
 *		in the tree -- if it is, H5SL_insert() will fail.
 *
 *		QAK -- 11/27/04
 *		Switched over to using skip list routines.
 *
 *		JRM -- 6/27/06
 *		Added fail_val parameter.
 *
 *		JRM -- 8/25/06
 *		Added the H5C_DO_SANITY_CHECKS version of the macro.
 *
 *		This version maintains the slist_len_increase and
 *		slist_size_increase fields that are used in sanity
 *		checks in the flush routines.
 *
 *		All this is needed as the fractal heap needs to be
 *		able to dirty, resize and/or move entries during the
 *		flush.
 *
 *		JRM -- 12/13/14
 *		Added code to set cache_ptr->slist_changed to TRUE 
 *		when an entry is inserted in the slist.
 *
 *		JRM -- 9/1/15
 *		Added code to maintain the cache_ptr->slist_ring_len
 *		and cache_ptr->slist_ring_size arrays.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_DO_SLIST_SANITY_CHECKS
#define ENTRY_IN_SLIST(cache_ptr, entry_ptr) \
    H5C_entry_in_skip_list((cache_ptr), (entry_ptr))
#else /* H5C_DO_SLIST_SANITY_CHECKS */
#define ENTRY_IN_SLIST(cache_ptr, entry_ptr) FALSE
#endif /* H5C_DO_SLIST_SANITY_CHECKS */

#if H5C_DO_SANITY_CHECKS

#define H5C__INSERT_ENTRY_IN_SLIST(cache_ptr, entry_ptr, fail_val)             \
{                                                                              \
    HDassert( (cache_ptr) );                                                   \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                        \
    HDassert( (entry_ptr) );                                                   \
    HDassert( (entry_ptr)->size > 0 );                                         \
    HDassert( H5F_addr_defined((entry_ptr)->addr) );                           \
    HDassert( !((entry_ptr)->in_slist) );                                      \
    HDassert( !ENTRY_IN_SLIST((cache_ptr), (entry_ptr)) );                     \
    HDassert( (entry_ptr)->ring > H5C_RING_UNDEFINED );                        \
    HDassert( (entry_ptr)->ring < H5C_RING_NTYPES );                           \
    HDassert( (cache_ptr)->slist_ring_len[(entry_ptr)->ring] <=                \
              (cache_ptr)->slist_len );                                        \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr)->ring] <=               \
              (cache_ptr)->slist_size );                                       \
                                                                               \
    if(H5SL_insert((cache_ptr)->slist_ptr, entry_ptr, &(entry_ptr)->addr) < 0) \
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, (fail_val), "can't insert entry in skip list") \
                                                                               \
    (entry_ptr)->in_slist = TRUE;                                              \
    (cache_ptr)->slist_changed = TRUE;                                         \
    (cache_ptr)->slist_len++;                                                  \
    (cache_ptr)->slist_size += (entry_ptr)->size;                              \
    ((cache_ptr)->slist_ring_len[(entry_ptr)->ring])++;                        \
    ((cache_ptr)->slist_ring_size[(entry_ptr)->ring]) += (entry_ptr)->size;    \
    (cache_ptr)->slist_len_increase++;                                         \
    (cache_ptr)->slist_size_increase += (int64_t)((entry_ptr)->size);          \
                                                                               \
    HDassert( (cache_ptr)->slist_len > 0 );                                    \
    HDassert( (cache_ptr)->slist_size > 0 );                                   \
                                                                               \
} /* H5C__INSERT_ENTRY_IN_SLIST */

#else /* H5C_DO_SANITY_CHECKS */

#define H5C__INSERT_ENTRY_IN_SLIST(cache_ptr, entry_ptr, fail_val)             \
{                                                                              \
    HDassert( (cache_ptr) );                                                   \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                        \
    HDassert( (entry_ptr) );                                                   \
    HDassert( (entry_ptr)->size > 0 );                                         \
    HDassert( H5F_addr_defined((entry_ptr)->addr) );                           \
    HDassert( !((entry_ptr)->in_slist) );                                      \
    HDassert( !ENTRY_IN_SLIST((cache_ptr), (entry_ptr)) );                     \
    HDassert( (entry_ptr)->ring > H5C_RING_UNDEFINED );                        \
    HDassert( (entry_ptr)->ring < H5C_RING_NTYPES );                           \
    HDassert( (cache_ptr)->slist_ring_len[(entry_ptr)->ring] <=                \
              (cache_ptr)->slist_len );                                        \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr)->ring] <=               \
              (cache_ptr)->slist_size );                                       \
                                                                               \
    if(H5SL_insert((cache_ptr)->slist_ptr, entry_ptr, &(entry_ptr)->addr) < 0) \
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, (fail_val), "can't insert entry in skip list") \
                                                                               \
    (entry_ptr)->in_slist = TRUE;                                              \
    (cache_ptr)->slist_changed = TRUE;                                         \
    (cache_ptr)->slist_len++;                                                  \
    (cache_ptr)->slist_size += (entry_ptr)->size;                              \
    ((cache_ptr)->slist_ring_len[(entry_ptr)->ring])++;                        \
    ((cache_ptr)->slist_ring_size[(entry_ptr)->ring]) += (entry_ptr)->size;    \
                                                                               \
    HDassert( (cache_ptr)->slist_len > 0 );                                    \
    HDassert( (cache_ptr)->slist_size > 0 );                                   \
                                                                               \
} /* H5C__INSERT_ENTRY_IN_SLIST */

#endif /* H5C_DO_SANITY_CHECKS */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__REMOVE_ENTRY_FROM_SLIST
 *
 * Purpose:     Remove the specified instance of H5C_cache_entry_t from the
 *		index skip list in the specified instance of H5C_t.  Update
 *		the associated length and size fields.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 5/10/04
 *
 *-------------------------------------------------------------------------
 */

#if H5C_DO_SANITY_CHECKS
#define H5C__REMOVE_ENTRY_FROM_SLIST(cache_ptr, entry_ptr, during_flush)    \
{                                                                           \
    HDassert( (cache_ptr) );                                                \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                     \
    HDassert( (entry_ptr) );                                                \
    HDassert( !((entry_ptr)->is_read_only) );                               \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                           \
    HDassert( (entry_ptr)->size > 0 );                                      \
    HDassert( (entry_ptr)->in_slist );                                      \
    HDassert( (cache_ptr)->slist_ptr );                                     \
    HDassert( (entry_ptr)->ring > H5C_RING_UNDEFINED );                     \
    HDassert( (entry_ptr)->ring < H5C_RING_NTYPES );                        \
    HDassert( (cache_ptr)->slist_ring_len[(entry_ptr)->ring] <=             \
              (cache_ptr)->slist_len );                                     \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr)->ring] <=            \
              (cache_ptr)->slist_size );                                    \
                                                                            \
    if ( H5SL_remove((cache_ptr)->slist_ptr, &(entry_ptr)->addr)            \
             != (entry_ptr) )                                               \
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "can't delete entry from skip list") \
                                                                            \
    HDassert( (cache_ptr)->slist_len > 0 );                                 \
    if(!(during_flush))                                                     \
        (cache_ptr)->slist_changed = TRUE;                                  \
    (cache_ptr)->slist_len--;                                               \
    HDassert( (cache_ptr)->slist_size >= (entry_ptr)->size );               \
    (cache_ptr)->slist_size -= (entry_ptr)->size;                           \
    ((cache_ptr)->slist_ring_len[(entry_ptr)->ring])--;                     \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr->ring)] >=            \
              (entry_ptr)->size );                                          \
    ((cache_ptr)->slist_ring_size[(entry_ptr)->ring]) -= (entry_ptr)->size; \
    (cache_ptr)->slist_len_increase--;                                      \
    (cache_ptr)->slist_size_increase -= (int64_t)((entry_ptr)->size);       \
    (entry_ptr)->in_slist = FALSE;                                          \
} /* H5C__REMOVE_ENTRY_FROM_SLIST */

#else /* H5C_DO_SANITY_CHECKS */

#define H5C__REMOVE_ENTRY_FROM_SLIST(cache_ptr, entry_ptr, during_flush)    \
{                                                                           \
    HDassert( (cache_ptr) );                                                \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                     \
    HDassert( (entry_ptr) );                                                \
    HDassert( !((entry_ptr)->is_read_only) );                               \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                           \
    HDassert( (entry_ptr)->in_slist );                                      \
    HDassert( (cache_ptr)->slist_ptr );                                     \
    HDassert( (entry_ptr)->ring > H5C_RING_UNDEFINED );                     \
    HDassert( (entry_ptr)->ring < H5C_RING_NTYPES );                        \
    HDassert( (cache_ptr)->slist_ring_len[(entry_ptr)->ring] <=             \
              (cache_ptr)->slist_len );                                     \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr)->ring] <=            \
              (cache_ptr)->slist_size );                                    \
                                                                            \
    if ( H5SL_remove((cache_ptr)->slist_ptr, &(entry_ptr)->addr)            \
             != (entry_ptr) )                                               \
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "can't delete entry from skip list") \
                                                                            \
    HDassert( (cache_ptr)->slist_len > 0 );                                 \
    if(!(during_flush))                                                     \
        (cache_ptr)->slist_changed = TRUE;                                  \
    (cache_ptr)->slist_len--;                                               \
    HDassert( (cache_ptr)->slist_size >= (entry_ptr)->size );               \
    (cache_ptr)->slist_size -= (entry_ptr)->size;                           \
    ((cache_ptr)->slist_ring_len[(entry_ptr)->ring])--;                     \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr->ring)] >=            \
              (entry_ptr)->size );                                          \
    ((cache_ptr)->slist_ring_size[(entry_ptr)->ring]) -= (entry_ptr)->size; \
    (entry_ptr)->in_slist = FALSE;                                          \
} /* H5C__REMOVE_ENTRY_FROM_SLIST */
#endif /* H5C_DO_SANITY_CHECKS */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__UPDATE_SLIST_FOR_SIZE_CHANGE
 *
 * Purpose:     Update cache_ptr->slist_size for a change in the size of
 *		and entry in the slist.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 9/07/05
 *
 * Modifications:
 *
 *		JRM -- 8/27/06
 *		Added the H5C_DO_SANITY_CHECKS version of the macro.
 *
 *		This version maintains the slist_size_increase field
 *		that are used in sanity checks in the flush routines.
 *
 *		All this is needed as the fractal heap needs to be
 *		able to dirty, resize and/or move entries during the
 *		flush.
 *
 *		JRM -- 12/13/14
 *		Note that we do not set cache_ptr->slist_changed to TRUE 
 *		in this case, as the structure of the slist is not
 *		modified.
 *
 *		JRM -- 9/1/15
 *		Added code to maintain the cache_ptr->slist_ring_len
 *		and cache_ptr->slist_ring_size arrays.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_DO_SANITY_CHECKS

#define H5C__UPDATE_SLIST_FOR_SIZE_CHANGE(cache_ptr, old_size, new_size)      \
{                                                                             \
    HDassert( (cache_ptr) );                                                  \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                       \
    HDassert( (old_size) > 0 );                                               \
    HDassert( (new_size) > 0 );                                               \
    HDassert( (old_size) <= (cache_ptr)->slist_size );                        \
    HDassert( (cache_ptr)->slist_len > 0 );                                   \
    HDassert( ((cache_ptr)->slist_len > 1) ||                                 \
              ( (cache_ptr)->slist_size == (old_size) ) );                    \
    HDassert( (entry_ptr)->ring > H5C_RING_UNDEFINED );                       \
    HDassert( (entry_ptr)->ring < H5C_RING_NTYPES );                          \
    HDassert( (cache_ptr)->slist_ring_len[(entry_ptr)->ring] <=               \
              (cache_ptr)->slist_len );                                       \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr)->ring] <=              \
              (cache_ptr)->slist_size );                                      \
                                                                              \
    (cache_ptr)->slist_size -= (old_size);                                    \
    (cache_ptr)->slist_size += (new_size);                                    \
                                                                              \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr->ring)] >=(old_size) ); \
    ((cache_ptr)->slist_ring_size[(entry_ptr)->ring]) -= (old_size);          \
    ((cache_ptr)->slist_ring_size[(entry_ptr)->ring]) += (new_size);          \
                                                                              \
    (cache_ptr)->slist_size_increase -= (int64_t)(old_size);                  \
    (cache_ptr)->slist_size_increase += (int64_t)(new_size);                  \
                                                                              \
    HDassert( (new_size) <= (cache_ptr)->slist_size );                        \
    HDassert( ( (cache_ptr)->slist_len > 1 ) ||                               \
              ( (cache_ptr)->slist_size == (new_size) ) );                    \
} /* H5C__UPDATE_SLIST_FOR_SIZE_CHANGE */

#else /* H5C_DO_SANITY_CHECKS */

#define H5C__UPDATE_SLIST_FOR_SIZE_CHANGE(cache_ptr, old_size, new_size)      \
{                                                                             \
    HDassert( (cache_ptr) );                                                  \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                       \
    HDassert( (old_size) > 0 );                                               \
    HDassert( (new_size) > 0 );                                               \
    HDassert( (old_size) <= (cache_ptr)->slist_size );                        \
    HDassert( (cache_ptr)->slist_len > 0 );                                   \
    HDassert( ((cache_ptr)->slist_len > 1) ||                                 \
              ( (cache_ptr)->slist_size == (old_size) ) );                    \
    HDassert( (entry_ptr)->ring > H5C_RING_UNDEFINED );                       \
    HDassert( (entry_ptr)->ring < H5C_RING_NTYPES );                          \
    HDassert( (cache_ptr)->slist_ring_len[(entry_ptr)->ring] <=               \
              (cache_ptr)->slist_len );                                       \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr)->ring] <=              \
              (cache_ptr)->slist_size );                                      \
                                                                              \
    (cache_ptr)->slist_size -= (old_size);                                    \
    (cache_ptr)->slist_size += (new_size);                                    \
                                                                              \
    HDassert( (cache_ptr)->slist_ring_size[(entry_ptr->ring)] >=(old_size) ); \
    ((cache_ptr)->slist_ring_size[(entry_ptr)->ring]) -= (old_size);          \
    ((cache_ptr)->slist_ring_size[(entry_ptr)->ring]) += (new_size);          \
                                                                              \
    HDassert( (new_size) <= (cache_ptr)->slist_size );                        \
    HDassert( ( (cache_ptr)->slist_len > 1 ) ||                               \
              ( (cache_ptr)->slist_size == (new_size) ) );                    \
} /* H5C__UPDATE_SLIST_FOR_SIZE_CHANGE */

#endif /* H5C_DO_SANITY_CHECKS */


/**************************************************************************
 *
 * Replacement policy update macros:
 *
 * These used to be functions, but I converted them to macros to avoid some
 * function call overhead.
 *
 **************************************************************************/

/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__FAKE_RP_FOR_MOST_RECENT_ACCESS
 *
 * Purpose:     For efficiency, we sometimes change the order of flushes --
 *		but doing so can confuse the replacement policy.  This
 *		macro exists to allow us to specify an entry as the
 *		most recently touched so we can repair any such
 *		confusion.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the macro
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 10/13/05
 *
 * Modifications:
 *
 *		JRM -- 3/20/06
 *		Modified macro to ignore pinned entries.  Pinned entries
 *		do not appear in the data structures maintained by the
 *		replacement policy code, and thus this macro has nothing
 *		to do if called for such an entry.
 *
 *		JRM -- 3/28/07
 *		Added sanity checks using the new is_read_only and
 *		ro_ref_count fields of struct H5C_cache_entry_t.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__FAKE_RP_FOR_MOST_RECENT_ACCESS(cache_ptr, entry_ptr, fail_val) \
{                                                                           \
    HDassert( (cache_ptr) );                                                \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                     \
    HDassert( (entry_ptr) );                                                \
    HDassert( !((entry_ptr)->is_protected) );                               \
    HDassert( !((entry_ptr)->is_read_only) );                               \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                           \
    HDassert( (entry_ptr)->size > 0 );                                      \
                                                                            \
    if ( ! ((entry_ptr)->is_pinned) ) {                                     \
                                                                            \
        /* modified LRU specific code */                                    \
                                                                            \
        /* remove the entry from the LRU list, and re-insert it at the head.\
	 */                                                                 \
                                                                            \
        H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,             \
                        (cache_ptr)->LRU_tail_ptr,                          \
			(cache_ptr)->LRU_list_len,                          \
                        (cache_ptr)->LRU_list_size, (fail_val))             \
                                                                            \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,            \
                         (cache_ptr)->LRU_tail_ptr,                         \
			 (cache_ptr)->LRU_list_len,                         \
                         (cache_ptr)->LRU_list_size, (fail_val))            \
                                                                            \
        /* Use the dirty flag to infer whether the entry is on the clean or \
         * dirty LRU list, and remove it.  Then insert it at the head of    \
         * the same LRU list.                                               \
         *                                                                  \
         * At least initially, all entries should be clean.  That may       \
         * change, so we may as well deal with both cases now.              \
         */                                                                 \
                                                                            \
        if ( (entry_ptr)->is_dirty ) {                                      \
            H5C__AUX_DLL_REMOVE((entry_ptr), (cache_ptr)->dLRU_head_ptr,    \
                                (cache_ptr)->dLRU_tail_ptr,                 \
                                (cache_ptr)->dLRU_list_len,                 \
                                (cache_ptr)->dLRU_list_size, (fail_val))    \
                                                                            \
            H5C__AUX_DLL_PREPEND((entry_ptr), (cache_ptr)->dLRU_head_ptr,   \
                                 (cache_ptr)->dLRU_tail_ptr,                \
                                 (cache_ptr)->dLRU_list_len,                \
                                 (cache_ptr)->dLRU_list_size, (fail_val))   \
        } else {                                                            \
            H5C__AUX_DLL_REMOVE((entry_ptr), (cache_ptr)->cLRU_head_ptr,    \
                                (cache_ptr)->cLRU_tail_ptr,                 \
                                (cache_ptr)->cLRU_list_len,                 \
                                (cache_ptr)->cLRU_list_size, (fail_val))    \
                                                                            \
            H5C__AUX_DLL_PREPEND((entry_ptr), (cache_ptr)->cLRU_head_ptr,   \
                                 (cache_ptr)->cLRU_tail_ptr,                \
                                 (cache_ptr)->cLRU_list_len,                \
                                 (cache_ptr)->cLRU_list_size, (fail_val))   \
        }                                                                   \
                                                                            \
        /* End modified LRU specific code. */                               \
    }                                                                       \
} /* H5C__FAKE_RP_FOR_MOST_RECENT_ACCESS */

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__FAKE_RP_FOR_MOST_RECENT_ACCESS(cache_ptr, entry_ptr, fail_val) \
{                                                                           \
    HDassert( (cache_ptr) );                                                \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                     \
    HDassert( (entry_ptr) );                                                \
    HDassert( !((entry_ptr)->is_protected) );                               \
    HDassert( !((entry_ptr)->is_read_only) );                               \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                           \
    HDassert( (entry_ptr)->size > 0 );                                      \
                                                                            \
    if ( ! ((entry_ptr)->is_pinned) ) {                                     \
                                                                            \
        /* modified LRU specific code */                                    \
                                                                            \
        /* remove the entry from the LRU list, and re-insert it at the head \
	 */                                                                 \
                                                                            \
        H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,             \
                        (cache_ptr)->LRU_tail_ptr,                          \
			(cache_ptr)->LRU_list_len,                          \
                        (cache_ptr)->LRU_list_size, (fail_val))             \
                                                                            \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,            \
                         (cache_ptr)->LRU_tail_ptr,                         \
			 (cache_ptr)->LRU_list_len,                         \
                         (cache_ptr)->LRU_list_size, (fail_val))            \
                                                                            \
        /* End modified LRU specific code. */                               \
    }                                                                       \
} /* H5C__FAKE_RP_FOR_MOST_RECENT_ACCESS */

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_EVICTION
 *
 * Purpose:     Update the replacement policy data structures for an
 *		eviction of the specified cache entry.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the function
 *		should switch on the current policy and act accordingly.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 5/10/04
 *
 * Modifications:
 *
 *		JRM - 7/27/04
 *		Converted the function H5C_update_rp_for_eviction() to the
 *		macro H5C__UPDATE_RP_FOR_EVICTION in an effort to squeeze
 *		a bit more performance out of the cache.
 *
 *		At least for the first cut, I am leaving the comments and
 *		white space in the macro.  If they cause difficulties with
 *		the pre-processor, I'll have to remove them.
 *
 *		JRM - 7/28/04
 *		Split macro into two version, one supporting the clean and
 *		dirty LRU lists, and the other not.  Yet another attempt
 *		at optimization.
 *
 *		JRM - 3/20/06
 *		Pinned entries can't be evicted, so this entry should never
 *		be called on a pinned entry.  Added assert to verify this.
 *
 *		JRM -- 3/28/07
 *		Added sanity checks for the new is_read_only and
 *		ro_ref_count fields of struct H5C_cache_entry_t.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_EVICTION(cache_ptr, entry_ptr, fail_val)          \
{                                                                            \
    HDassert( (cache_ptr) );                                                 \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                      \
    HDassert( (entry_ptr) );                                                 \
    HDassert( !((entry_ptr)->is_protected) );                                \
    HDassert( !((entry_ptr)->is_read_only) );                                \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                            \
    HDassert( !((entry_ptr)->is_pinned) );                                   \
    HDassert( (entry_ptr)->size > 0 );                                       \
                                                                             \
    /* modified LRU specific code */                                         \
                                                                             \
    /* remove the entry from the LRU list. */                                \
                                                                             \
    H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,                  \
                    (cache_ptr)->LRU_tail_ptr, (cache_ptr)->LRU_list_len,    \
                    (cache_ptr)->LRU_list_size, (fail_val))                  \
                                                                             \
    /* If the entry is clean when it is evicted, it should be on the         \
     * clean LRU list, if it was dirty, it should be on the dirty LRU list.  \
     * Remove it from the appropriate list according to the value of the     \
     * dirty flag.                                                           \
     */                                                                      \
                                                                             \
    if ( (entry_ptr)->is_dirty ) {                                           \
                                                                             \
        H5C__AUX_DLL_REMOVE((entry_ptr), (cache_ptr)->dLRU_head_ptr,         \
                            (cache_ptr)->dLRU_tail_ptr,                      \
                            (cache_ptr)->dLRU_list_len,                      \
                            (cache_ptr)->dLRU_list_size, (fail_val))         \
    } else {                                                                 \
        H5C__AUX_DLL_REMOVE((entry_ptr), (cache_ptr)->cLRU_head_ptr,         \
                            (cache_ptr)->cLRU_tail_ptr,                      \
                            (cache_ptr)->cLRU_list_len,                      \
                            (cache_ptr)->cLRU_list_size, (fail_val))         \
    }                                                                        \
                                                                             \
} /* H5C__UPDATE_RP_FOR_EVICTION */

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_EVICTION(cache_ptr, entry_ptr, fail_val)          \
{                                                                            \
    HDassert( (cache_ptr) );                                                 \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                      \
    HDassert( (entry_ptr) );                                                 \
    HDassert( !((entry_ptr)->is_protected) );                                \
    HDassert( !((entry_ptr)->is_read_only) );                                \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                            \
    HDassert( !((entry_ptr)->is_pinned) );                                   \
    HDassert( (entry_ptr)->size > 0 );                                       \
                                                                             \
    /* modified LRU specific code */                                         \
                                                                             \
    /* remove the entry from the LRU list. */                                \
                                                                             \
    H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,                  \
                    (cache_ptr)->LRU_tail_ptr, (cache_ptr)->LRU_list_len,    \
                    (cache_ptr)->LRU_list_size, (fail_val))                  \
                                                                             \
} /* H5C__UPDATE_RP_FOR_EVICTION */

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_FLUSH
 *
 * Purpose:     Update the replacement policy data structures for a flush
 *		of the specified cache entry.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the function
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 5/6/04
 *
 * Modifications:
 *
 *		JRM - 7/27/04
 *		Converted the function H5C_update_rp_for_flush() to the
 *		macro H5C__UPDATE_RP_FOR_FLUSH in an effort to squeeze
 *		a bit more performance out of the cache.
 *
 *		At least for the first cut, I am leaving the comments and
 *		white space in the macro.  If they cause difficulties with
 *		pre-processor, I'll have to remove them.
 *
 *		JRM - 7/28/04
 *		Split macro into two versions, one supporting the clean and
 *		dirty LRU lists, and the other not.  Yet another attempt
 *		at optimization.
 *
 *		JRM - 3/20/06
 *		While pinned entries can be flushed, they don't reside in
 *		the replacement policy data structures when unprotected.
 *		Thus I modified this macro to do nothing if the entry is
 *		pinned.
 *
 *		JRM - 3/28/07
 *		Added sanity checks based on the new is_read_only and
 *		ro_ref_count fields of struct H5C_cache_entry_t.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_FLUSH(cache_ptr, entry_ptr, fail_val)            \
{                                                                           \
    HDassert( (cache_ptr) );                                                \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                     \
    HDassert( (entry_ptr) );                                                \
    HDassert( !((entry_ptr)->is_protected) );                               \
    HDassert( !((entry_ptr)->is_read_only) );                               \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                           \
    HDassert( (entry_ptr)->size > 0 );                                      \
                                                                            \
    if ( ! ((entry_ptr)->is_pinned) ) {                                     \
                                                                            \
        /* modified LRU specific code */                                    \
                                                                            \
        /* remove the entry from the LRU list, and re-insert it at the      \
	 * head.                                                            \
	 */                                                                 \
                                                                            \
        H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,             \
                        (cache_ptr)->LRU_tail_ptr,                          \
			(cache_ptr)->LRU_list_len,                          \
                        (cache_ptr)->LRU_list_size, (fail_val))             \
                                                                            \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,            \
                         (cache_ptr)->LRU_tail_ptr,                         \
			 (cache_ptr)->LRU_list_len,                         \
                         (cache_ptr)->LRU_list_size, (fail_val))            \
                                                                            \
        /* since the entry is being flushed or cleared, one would think     \
	 * that it must be dirty -- but that need not be the case.  Use the \
	 * dirty flag to infer whether the entry is on the clean or dirty   \
	 * LRU list, and remove it.  Then insert it at the head of the      \
	 * clean LRU list.                                                  \
         *                                                                  \
         * The function presumes that a dirty entry will be either cleared  \
	 * or flushed shortly, so it is OK if we put a dirty entry on the   \
	 * clean LRU list.                                                  \
         */                                                                 \
                                                                            \
        if ( (entry_ptr)->is_dirty ) {                                      \
            H5C__AUX_DLL_REMOVE((entry_ptr), (cache_ptr)->dLRU_head_ptr,    \
                                (cache_ptr)->dLRU_tail_ptr,                 \
                                (cache_ptr)->dLRU_list_len,                 \
                                (cache_ptr)->dLRU_list_size, (fail_val))    \
        } else {                                                            \
            H5C__AUX_DLL_REMOVE((entry_ptr), (cache_ptr)->cLRU_head_ptr,    \
                                (cache_ptr)->cLRU_tail_ptr,                 \
                                (cache_ptr)->cLRU_list_len,                 \
                                (cache_ptr)->cLRU_list_size, (fail_val))    \
        }                                                                   \
                                                                            \
        H5C__AUX_DLL_PREPEND((entry_ptr), (cache_ptr)->cLRU_head_ptr,       \
                             (cache_ptr)->cLRU_tail_ptr,                    \
                             (cache_ptr)->cLRU_list_len,                    \
                             (cache_ptr)->cLRU_list_size, (fail_val))       \
                                                                            \
        /* End modified LRU specific code. */                               \
    }                                                                       \
} /* H5C__UPDATE_RP_FOR_FLUSH */

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_FLUSH(cache_ptr, entry_ptr, fail_val)            \
{                                                                           \
    HDassert( (cache_ptr) );                                                \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                     \
    HDassert( (entry_ptr) );                                                \
    HDassert( !((entry_ptr)->is_protected) );                               \
    HDassert( !((entry_ptr)->is_read_only) );                               \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                           \
    HDassert( (entry_ptr)->size > 0 );                                      \
                                                                            \
    if ( ! ((entry_ptr)->is_pinned) ) {                                     \
                                                                            \
        /* modified LRU specific code */                                    \
                                                                            \
        /* remove the entry from the LRU list, and re-insert it at the      \
	 * head.                                                            \
	 */                                                                 \
                                                                            \
        H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,             \
                        (cache_ptr)->LRU_tail_ptr,                          \
			(cache_ptr)->LRU_list_len,                          \
                        (cache_ptr)->LRU_list_size, (fail_val))             \
                                                                            \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,            \
                         (cache_ptr)->LRU_tail_ptr,                         \
			 (cache_ptr)->LRU_list_len,                         \
                         (cache_ptr)->LRU_list_size, (fail_val))            \
                                                                            \
        /* End modified LRU specific code. */                               \
    }                                                                       \
} /* H5C__UPDATE_RP_FOR_FLUSH */

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_INSERT_APPEND
 *
 * Purpose:     Update the replacement policy data structures for an
 *		insertion of the specified cache entry.  
 *
 *		Unlike H5C__UPDATE_RP_FOR_INSERTION below, mark the 
 *		new entry as the LEAST recently used entry, not the 
 *		most recently used.  
 *
 *		For now at least, this macro should only be used in 
 *		the reconstruction of the metadata cache from a cache 
 *		image block.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the function
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 8/15/15
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_INSERT_APPEND(cache_ptr, entry_ptr, fail_val)   \
{                                                                          \
    HDassert( (cache_ptr) );                                               \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                    \
    HDassert( (entry_ptr) );                                               \
    HDassert( !((entry_ptr)->is_protected) );                              \
    HDassert( !((entry_ptr)->is_read_only) );                              \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                          \
    HDassert( (entry_ptr)->size > 0 );                                     \
                                                                           \
    if ( (entry_ptr)->is_pinned ) {                                        \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->pel_head_ptr,           \
                         (cache_ptr)->pel_tail_ptr,                        \
                         (cache_ptr)->pel_len,                             \
                         (cache_ptr)->pel_size, (fail_val))                \
                                                                           \
    } else {                                                               \
                                                                           \
        /* modified LRU specific code */                                   \
                                                                           \
        /* insert the entry at the tail of the LRU list. */                \
                                                                           \
        H5C__DLL_APPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,            \
                        (cache_ptr)->LRU_tail_ptr,                         \
		        (cache_ptr)->LRU_list_len,                         \
                        (cache_ptr)->LRU_list_size, (fail_val))            \
                                                                           \
        /* insert the entry at the tail of the clean or dirty LRU list as  \
         * appropriate.                                                    \
         */                                                                \
                                                                           \
        if ( entry_ptr->is_dirty ) {                                       \
            H5C__AUX_DLL_APPEND((entry_ptr), (cache_ptr)->dLRU_head_ptr,   \
                                (cache_ptr)->dLRU_tail_ptr,                \
                                (cache_ptr)->dLRU_list_len,                \
                                (cache_ptr)->dLRU_list_size, (fail_val))   \
        } else {                                                           \
            H5C__AUX_DLL_APPEND((entry_ptr), (cache_ptr)->cLRU_head_ptr,   \
                                (cache_ptr)->cLRU_tail_ptr,                \
                                (cache_ptr)->cLRU_list_len,                \
                                (cache_ptr)->cLRU_list_size, (fail_val))   \
        }                                                                  \
                                                                           \
        /* End modified LRU specific code. */                              \
    }                                                                      \
}

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_INSERT_APPEND(cache_ptr, entry_ptr, fail_val)   \
{                                                                          \
    HDassert( (cache_ptr) );                                               \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                    \
    HDassert( (entry_ptr) );                                               \
    HDassert( !((entry_ptr)->is_protected) );                              \
    HDassert( !((entry_ptr)->is_read_only) );                              \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                          \
    HDassert( (entry_ptr)->size > 0 );                                     \
                                                                           \
    if ( (entry_ptr)->is_pinned ) {                                        \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->pel_head_ptr,           \
                         (cache_ptr)->pel_tail_ptr,                        \
                         (cache_ptr)->pel_len,                             \
                         (cache_ptr)->pel_size, (fail_val))                \
	                                                                   \
    } else {                                                               \
                                                                           \
        /* modified LRU specific code */                                   \
                                                                           \
        /* insert the entry at the tail of the LRU list. */                \
                                                                           \
        H5C__DLL_APPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,            \
                        (cache_ptr)->LRU_tail_ptr,                         \
			(cache_ptr)->LRU_list_len,                         \
                        (cache_ptr)->LRU_list_size, (fail_val))            \
                                                                           \
        /* End modified LRU specific code. */                              \
    }                                                                      \
}

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_INSERTION
 *
 * Purpose:     Update the replacement policy data structures for an
 *		insertion of the specified cache entry.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the function
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 5/17/04
 *
 * Modifications:
 *
 *		JRM - 7/27/04
 *		Converted the function H5C_update_rp_for_insertion() to the
 *		macro H5C__UPDATE_RP_FOR_INSERTION in an effort to squeeze
 *		a bit more performance out of the cache.
 *
 *		At least for the first cut, I am leaving the comments and
 *		white space in the macro.  If they cause difficulties with
 *		pre-processor, I'll have to remove them.
 *
 *		JRM - 7/28/04
 *		Split macro into two version, one supporting the clean and
 *		dirty LRU lists, and the other not.  Yet another attempt
 *		at optimization.
 *
 *		JRM - 3/10/06
 *		This macro should never be called on a pinned entry.
 *		Inserted an assert to verify this.
 *
 *		JRM - 8/9/06
 *		Not any more.  We must now allow insertion of pinned
 *		entries.  Updated macro to support this.
 *
 *		JRM - 3/28/07
 *		Added sanity checks using the new is_read_only and
 *		ro_ref_count fields of struct H5C_cache_entry_t.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_INSERTION(cache_ptr, entry_ptr, fail_val)       \
{                                                                          \
    HDassert( (cache_ptr) );                                               \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                    \
    HDassert( (entry_ptr) );                                               \
    HDassert( !((entry_ptr)->is_protected) );                              \
    HDassert( !((entry_ptr)->is_read_only) );                              \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                          \
    HDassert( (entry_ptr)->size > 0 );                                     \
                                                                           \
    if ( (entry_ptr)->is_pinned ) {                                        \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->pel_head_ptr,           \
                         (cache_ptr)->pel_tail_ptr,                        \
                         (cache_ptr)->pel_len,                             \
                         (cache_ptr)->pel_size, (fail_val))                \
                                                                           \
    } else {                                                               \
                                                                           \
        /* modified LRU specific code */                                   \
                                                                           \
        /* insert the entry at the head of the LRU list. */                \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,           \
                         (cache_ptr)->LRU_tail_ptr,                        \
			 (cache_ptr)->LRU_list_len,                        \
                         (cache_ptr)->LRU_list_size, (fail_val))           \
                                                                           \
        /* insert the entry at the head of the clean or dirty LRU list as  \
         * appropriate.                                                    \
         */                                                                \
                                                                           \
        if ( entry_ptr->is_dirty ) {                                       \
            H5C__AUX_DLL_PREPEND((entry_ptr), (cache_ptr)->dLRU_head_ptr,  \
                                 (cache_ptr)->dLRU_tail_ptr,               \
                                 (cache_ptr)->dLRU_list_len,               \
                                 (cache_ptr)->dLRU_list_size, (fail_val))  \
        } else {                                                           \
            H5C__AUX_DLL_PREPEND((entry_ptr), (cache_ptr)->cLRU_head_ptr,  \
                                 (cache_ptr)->cLRU_tail_ptr,               \
                                 (cache_ptr)->cLRU_list_len,               \
                                 (cache_ptr)->cLRU_list_size, (fail_val))  \
        }                                                                  \
                                                                           \
        /* End modified LRU specific code. */                              \
    }                                                                      \
}

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_INSERTION(cache_ptr, entry_ptr, fail_val)       \
{                                                                          \
    HDassert( (cache_ptr) );                                               \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                    \
    HDassert( (entry_ptr) );                                               \
    HDassert( !((entry_ptr)->is_protected) );                              \
    HDassert( !((entry_ptr)->is_read_only) );                              \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                          \
    HDassert( (entry_ptr)->size > 0 );                                     \
                                                                           \
    if ( (entry_ptr)->is_pinned ) {                                        \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->pel_head_ptr,           \
                         (cache_ptr)->pel_tail_ptr,                        \
                         (cache_ptr)->pel_len,                             \
                         (cache_ptr)->pel_size, (fail_val))                \
	                                                                   \
    } else {                                                               \
                                                                           \
        /* modified LRU specific code */                                   \
                                                                           \
        /* insert the entry at the head of the LRU list. */                \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,           \
                         (cache_ptr)->LRU_tail_ptr,                        \
			 (cache_ptr)->LRU_list_len,                        \
                         (cache_ptr)->LRU_list_size, (fail_val))           \
                                                                           \
        /* End modified LRU specific code. */                              \
    }                                                                      \
}

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_PROTECT
 *
 * Purpose:     Update the replacement policy data structures for a
 *		protect of the specified cache entry.
 *
 *		To do this, unlink the specified entry from any data
 *		structures used by the replacement policy, and add the
 *		entry to the protected list.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the function
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 5/17/04
 *
 * Modifications:
 *
 *		JRM - 7/27/04
 *		Converted the function H5C_update_rp_for_protect() to the
 *		macro H5C__UPDATE_RP_FOR_PROTECT in an effort to squeeze
 *		a bit more performance out of the cache.
 *
 *		At least for the first cut, I am leaving the comments and
 *		white space in the macro.  If they cause difficulties with
 *		pre-processor, I'll have to remove them.
 *
 *		JRM - 7/28/04
 *		Split macro into two version, one supporting the clean and
 *		dirty LRU lists, and the other not.  Yet another attempt
 *		at optimization.
 *
 *		JRM - 3/17/06
 *		Modified macro to attempt to remove pinned entriese from
 *		the pinned entry list instead of from the data structures
 *		maintained by the replacement policy.
 *
 *		JRM - 3/28/07
 *		Added sanity checks based on the new is_read_only and
 *		ro_ref_count fields of struct H5C_cache_entry_t.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_PROTECT(cache_ptr, entry_ptr, fail_val)        \
{                                                                         \
    HDassert( (cache_ptr) );                                              \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                   \
    HDassert( (entry_ptr) );                                              \
    HDassert( !((entry_ptr)->is_protected) );                             \
    HDassert( !((entry_ptr)->is_read_only) );                             \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                         \
    HDassert( (entry_ptr)->size > 0 );                                    \
									  \
    if ( (entry_ptr)->is_pinned ) {                                       \
                                                                          \
        H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->pel_head_ptr,           \
                        (cache_ptr)->pel_tail_ptr, 			  \
			(cache_ptr)->pel_len,                             \
                        (cache_ptr)->pel_size, (fail_val))                \
                                                                          \
    } else {                                                              \
                                                                          \
        /* modified LRU specific code */                                  \
                                                                          \
        /* remove the entry from the LRU list. */                         \
                                                                          \
        H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,           \
                        (cache_ptr)->LRU_tail_ptr,                        \
			(cache_ptr)->LRU_list_len,                        \
                        (cache_ptr)->LRU_list_size, (fail_val))           \
                                                                          \
        /* Similarly, remove the entry from the clean or dirty LRU list   \
         * as appropriate.                                                \
         */                                                               \
                                                                          \
        if ( (entry_ptr)->is_dirty ) {                                    \
                                                                          \
            H5C__AUX_DLL_REMOVE((entry_ptr), (cache_ptr)->dLRU_head_ptr,  \
                                (cache_ptr)->dLRU_tail_ptr,               \
                                (cache_ptr)->dLRU_list_len,               \
                                (cache_ptr)->dLRU_list_size, (fail_val))  \
                                                                          \
        } else {                                                          \
                                                                          \
            H5C__AUX_DLL_REMOVE((entry_ptr), (cache_ptr)->cLRU_head_ptr,  \
                                (cache_ptr)->cLRU_tail_ptr,               \
                                (cache_ptr)->cLRU_list_len,               \
                                (cache_ptr)->cLRU_list_size, (fail_val))  \
        }                                                                 \
                                                                          \
        /* End modified LRU specific code. */                             \
    }                                                                     \
                                                                          \
    /* Regardless of the replacement policy, or whether the entry is      \
     * pinned, now add the entry to the protected list.                   \
     */                                                                   \
                                                                          \
    H5C__DLL_APPEND((entry_ptr), (cache_ptr)->pl_head_ptr,                \
                    (cache_ptr)->pl_tail_ptr,                             \
                    (cache_ptr)->pl_len,                                  \
                    (cache_ptr)->pl_size, (fail_val))                     \
} /* H5C__UPDATE_RP_FOR_PROTECT */

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_PROTECT(cache_ptr, entry_ptr, fail_val)        \
{                                                                         \
    HDassert( (cache_ptr) );                                              \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                   \
    HDassert( (entry_ptr) );                                              \
    HDassert( !((entry_ptr)->is_protected) );                             \
    HDassert( !((entry_ptr)->is_read_only) );                             \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                         \
    HDassert( (entry_ptr)->size > 0 );                                    \
									  \
    if ( (entry_ptr)->is_pinned ) {                                       \
                                                                          \
        H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->pel_head_ptr,           \
                        (cache_ptr)->pel_tail_ptr, 			  \
			(cache_ptr)->pel_len,                             \
                        (cache_ptr)->pel_size, (fail_val))                \
                                                                          \
    } else {                                                              \
                                                                          \
        /* modified LRU specific code */                                  \
                                                                          \
        /* remove the entry from the LRU list. */                         \
                                                                          \
        H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,           \
                        (cache_ptr)->LRU_tail_ptr,                        \
			(cache_ptr)->LRU_list_len,                        \
                        (cache_ptr)->LRU_list_size, (fail_val))           \
                                                                          \
        /* End modified LRU specific code. */                             \
    }                                                                     \
                                                                          \
    /* Regardless of the replacement policy, or whether the entry is      \
     * pinned, now add the entry to the protected list.                   \
     */                                                                   \
                                                                          \
    H5C__DLL_APPEND((entry_ptr), (cache_ptr)->pl_head_ptr,                \
                    (cache_ptr)->pl_tail_ptr,                             \
                    (cache_ptr)->pl_len,                                  \
                    (cache_ptr)->pl_size, (fail_val))                     \
} /* H5C__UPDATE_RP_FOR_PROTECT */

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_MOVE
 *
 * Purpose:     Update the replacement policy data structures for a
 *		move of the specified cache entry.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the function
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 5/17/04
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_MOVE(cache_ptr, entry_ptr, was_dirty, fail_val) \
{                                                                            \
    HDassert( (cache_ptr) );                                                 \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                      \
    HDassert( (entry_ptr) );                                                 \
    HDassert( !((entry_ptr)->is_read_only) );                                \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                            \
    HDassert( (entry_ptr)->size > 0 );                                       \
                                                                             \
    if ( ! ( (entry_ptr)->is_pinned ) && ! ( (entry_ptr->is_protected ) ) ) { \
	                                                                     \
        /* modified LRU specific code */                                     \
                                                                             \
        /* remove the entry from the LRU list, and re-insert it at the head. \
	 */                                                                  \
                                                                             \
            H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,          \
                             (cache_ptr)->LRU_tail_ptr,                      \
			     (cache_ptr)->LRU_list_len,                      \
                             (cache_ptr)->LRU_list_size, (fail_val))         \
                                                                             \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,             \
                         (cache_ptr)->LRU_tail_ptr,                          \
			 (cache_ptr)->LRU_list_len,                          \
                         (cache_ptr)->LRU_list_size, (fail_val))             \
                                                                             \
            /* remove the entry from either the clean or dirty LUR list as   \
             * indicated by the was_dirty parameter                          \
             */                                                              \
            if ( was_dirty ) {                                               \
                                                                             \
                H5C__AUX_DLL_REMOVE((entry_ptr),                             \
				     (cache_ptr)->dLRU_head_ptr,             \
                                     (cache_ptr)->dLRU_tail_ptr,             \
                                     (cache_ptr)->dLRU_list_len,             \
                                     (cache_ptr)->dLRU_list_size,            \
				     (fail_val))                             \
                                                                             \
            } else {                                                         \
                                                                             \
                H5C__AUX_DLL_REMOVE((entry_ptr),                             \
				     (cache_ptr)->cLRU_head_ptr,             \
                                     (cache_ptr)->cLRU_tail_ptr,             \
                                     (cache_ptr)->cLRU_list_len,             \
                                     (cache_ptr)->cLRU_list_size,            \
				     (fail_val))                             \
            }                                                                \
                                                                             \
            /* insert the entry at the head of either the clean or dirty     \
	     * LRU list as appropriate.                                      \
             */                                                              \
                                                                             \
            if ( (entry_ptr)->is_dirty ) {                                   \
                                                                             \
                H5C__AUX_DLL_PREPEND((entry_ptr),                            \
				      (cache_ptr)->dLRU_head_ptr,            \
                                      (cache_ptr)->dLRU_tail_ptr,            \
                                      (cache_ptr)->dLRU_list_len,            \
                                      (cache_ptr)->dLRU_list_size,           \
				      (fail_val))                            \
                                                                             \
            } else {                                                         \
                                                                             \
                H5C__AUX_DLL_PREPEND((entry_ptr),                            \
				      (cache_ptr)->cLRU_head_ptr,            \
                                      (cache_ptr)->cLRU_tail_ptr,            \
                                      (cache_ptr)->cLRU_list_len,            \
                                      (cache_ptr)->cLRU_list_size,           \
				      (fail_val))                            \
            }                                                                \
                                                                             \
            /* End modified LRU specific code. */                            \
        }                                                                    \
} /* H5C__UPDATE_RP_FOR_MOVE */

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_MOVE(cache_ptr, entry_ptr, was_dirty, fail_val) \
{                                                                            \
    HDassert( (cache_ptr) );                                                 \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                      \
    HDassert( (entry_ptr) );                                                 \
    HDassert( !((entry_ptr)->is_read_only) );                                \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                            \
    HDassert( (entry_ptr)->size > 0 );                                       \
                                                                             \
    if ( ! ( (entry_ptr)->is_pinned ) && ! ( (entry_ptr->is_protected ) ) ) { \
	                                                                     \
        /* modified LRU specific code */                                     \
                                                                             \
        /* remove the entry from the LRU list, and re-insert it at the head. \
	 */                                                                  \
                                                                             \
            H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->LRU_head_ptr,          \
                             (cache_ptr)->LRU_tail_ptr,                      \
			     (cache_ptr)->LRU_list_len,                      \
                             (cache_ptr)->LRU_list_size, (fail_val))         \
                                                                             \
            H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,         \
                              (cache_ptr)->LRU_tail_ptr,                     \
			      (cache_ptr)->LRU_list_len,                     \
                              (cache_ptr)->LRU_list_size, (fail_val))        \
                                                                             \
            /* End modified LRU specific code. */                            \
        }                                                                    \
} /* H5C__UPDATE_RP_FOR_MOVE */

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_SIZE_CHANGE
 *
 * Purpose:     Update the replacement policy data structures for a
 *		size change of the specified cache entry.
 *
 *		To do this, determine if the entry is pinned.  If it is,
 *		update the size of the pinned entry list.
 *
 *		If it isn't pinned, the entry must handled by the
 *		replacement policy.  Update the appropriate replacement
 *		policy data structures.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the function
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 8/23/06
 *
 * Modifications:
 *
 * 		JRM -- 3/28/07
 *		Added sanity checks based on the new is_read_only and
 *		ro_ref_count fields of struct H5C_cache_entry_t.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_SIZE_CHANGE(cache_ptr, entry_ptr, new_size)    \
{                                                                         \
    HDassert( (cache_ptr) );                                              \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                   \
    HDassert( (entry_ptr) );                                              \
    HDassert( !((entry_ptr)->is_protected) );                             \
    HDassert( !((entry_ptr)->is_read_only) );                             \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                         \
    HDassert( (entry_ptr)->size > 0 );                                    \
    HDassert( new_size > 0 );                                             \
                                                                          \
    if ( (entry_ptr)->coll_access ) {                                     \
                                                                          \
	H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr)->coll_list_len,       \
			                (cache_ptr)->coll_list_size,      \
			                (entry_ptr)->size,                \
					(new_size));                      \
	                                                                  \
    }                                                                     \
                                                                          \
    if ( (entry_ptr)->is_pinned ) {                                       \
                                                                          \
	H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr)->pel_len,             \
			                (cache_ptr)->pel_size,            \
			                (entry_ptr)->size,                \
					(new_size));                      \
	                                                                  \
    } else {                                                              \
                                                                          \
        /* modified LRU specific code */                                  \
                                                                          \
	/* Update the size of the LRU list */                             \
                                                                          \
	H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr)->LRU_list_len,        \
			                (cache_ptr)->LRU_list_size,       \
			                (entry_ptr)->size,                \
					(new_size));                      \
                                                                          \
        /* Similarly, update the size of the clean or dirty LRU list as   \
	 * appropriate.  At present, the entry must be clean, but that    \
	 * could change.                                                  \
         */                                                               \
                                                                          \
        if ( (entry_ptr)->is_dirty ) {                                    \
                                                                          \
	    H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr)->dLRU_list_len,   \
			                    (cache_ptr)->dLRU_list_size,  \
			                    (entry_ptr)->size,            \
					    (new_size));                  \
                                                                          \
        } else {                                                          \
                                                                          \
	    H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr)->cLRU_list_len,   \
			                    (cache_ptr)->cLRU_list_size,  \
			                    (entry_ptr)->size,            \
					    (new_size));                  \
        }                                                                 \
                                                                          \
        /* End modified LRU specific code. */                             \
    }                                                                     \
                                                                          \
} /* H5C__UPDATE_RP_FOR_SIZE_CHANGE */

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_SIZE_CHANGE(cache_ptr, entry_ptr, new_size)    \
{                                                                         \
    HDassert( (cache_ptr) );                                              \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                   \
    HDassert( (entry_ptr) );                                              \
    HDassert( !((entry_ptr)->is_protected) );                             \
    HDassert( !((entry_ptr)->is_read_only) );                             \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                         \
    HDassert( (entry_ptr)->size > 0 );                                    \
    HDassert( new_size > 0 );                                             \
				  					  \
    if ( (entry_ptr)->is_pinned ) {                                       \
                                                                          \
	H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr)->pel_len,             \
			                (cache_ptr)->pel_size,            \
			                (entry_ptr)->size,                \
					(new_size));                      \
                                                                          \
    } else {                                                              \
                                                                          \
        /* modified LRU specific code */                                  \
                                                                          \
	/* Update the size of the LRU list */                             \
                                                                          \
	H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr)->LRU_list_len,        \
			                (cache_ptr)->LRU_list_size,       \
			                (entry_ptr)->size,                \
					(new_size));                      \
                                                                          \
        /* End modified LRU specific code. */                             \
    }                                                                     \
                                                                          \
} /* H5C__UPDATE_RP_FOR_SIZE_CHANGE */

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_UNPIN
 *
 * Purpose:     Update the replacement policy data structures for an
 *		unpin of the specified cache entry.
 *
 *		To do this, unlink the specified entry from the protected
 *		entry list, and re-insert it in the data structures used
 *		by the current replacement policy.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the macro
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 3/22/06
 *
 * Modifications:
 *
 *		JRM -- 3/28/07
 *		Added sanity checks based on the new is_read_only and
 *		ro_ref_count fields of struct H5C_cache_entry_t.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_UNPIN(cache_ptr, entry_ptr, fail_val)       \
{                                                                      \
    HDassert( (cache_ptr) );                                           \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                \
    HDassert( (entry_ptr) );                                           \
    HDassert( !((entry_ptr)->is_protected) );                          \
    HDassert( !((entry_ptr)->is_read_only) );                          \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                      \
    HDassert( (entry_ptr)->is_pinned);                                 \
    HDassert( (entry_ptr)->size > 0 );                                 \
                                                                       \
    /* Regardless of the replacement policy, remove the entry from the \
     * pinned entry list.                                              \
     */                                                                \
    H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->pel_head_ptr,            \
                    (cache_ptr)->pel_tail_ptr, (cache_ptr)->pel_len,   \
                    (cache_ptr)->pel_size, (fail_val))                 \
                                                                       \
    /* modified LRU specific code */                                   \
                                                                       \
    /* insert the entry at the head of the LRU list. */                \
                                                                       \
    H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,           \
                     (cache_ptr)->LRU_tail_ptr,                        \
                     (cache_ptr)->LRU_list_len,                        \
                     (cache_ptr)->LRU_list_size, (fail_val))           \
                                                                       \
    /* Similarly, insert the entry at the head of either the clean     \
     * or dirty LRU list as appropriate.                               \
     */                                                                \
                                                                       \
    if ( (entry_ptr)->is_dirty ) {                                     \
                                                                       \
        H5C__AUX_DLL_PREPEND((entry_ptr),                              \
                              (cache_ptr)->dLRU_head_ptr,              \
                              (cache_ptr)->dLRU_tail_ptr,              \
                              (cache_ptr)->dLRU_list_len,              \
                              (cache_ptr)->dLRU_list_size,             \
                              (fail_val))                              \
                                                                       \
    } else {                                                           \
                                                                       \
        H5C__AUX_DLL_PREPEND((entry_ptr),                              \
                              (cache_ptr)->cLRU_head_ptr,              \
                              (cache_ptr)->cLRU_tail_ptr,              \
                              (cache_ptr)->cLRU_list_len,              \
                              (cache_ptr)->cLRU_list_size,             \
                              (fail_val))                              \
     }                                                                 \
                                                                       \
    /* End modified LRU specific code. */                              \
                                                                       \
} /* H5C__UPDATE_RP_FOR_UNPIN */

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_UNPIN(cache_ptr, entry_ptr, fail_val)       \
{                                                                      \
    HDassert( (cache_ptr) );                                           \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                \
    HDassert( (entry_ptr) );                                           \
    HDassert( !((entry_ptr)->is_protected) );                          \
    HDassert( !((entry_ptr)->is_read_only) );                          \
    HDassert( ((entry_ptr)->ro_ref_count) == 0 );                      \
    HDassert( (entry_ptr)->is_pinned);                                 \
    HDassert( (entry_ptr)->size > 0 );                                 \
                                                                       \
    /* Regardless of the replacement policy, remove the entry from the \
     * pinned entry list.                                              \
     */                                                                \
    H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->pel_head_ptr,            \
                    (cache_ptr)->pel_tail_ptr, (cache_ptr)->pel_len,   \
                    (cache_ptr)->pel_size, (fail_val))                 \
                                                                       \
        /* modified LRU specific code */                               \
                                                                       \
        /* insert the entry at the head of the LRU list. */            \
                                                                       \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,       \
                         (cache_ptr)->LRU_tail_ptr,                    \
                         (cache_ptr)->LRU_list_len,                    \
                         (cache_ptr)->LRU_list_size, (fail_val))       \
                                                                       \
        /* End modified LRU specific code. */                          \
                                                                       \
} /* H5C__UPDATE_RP_FOR_UNPIN */

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__UPDATE_RP_FOR_UNPROTECT
 *
 * Purpose:     Update the replacement policy data structures for an
 *		unprotect of the specified cache entry.
 *
 *		To do this, unlink the specified entry from the protected
 *		list, and re-insert it in the data structures used by the
 *		current replacement policy.
 *
 *		At present, we only support the modified LRU policy, so
 *		this function deals with that case unconditionally.  If
 *		we ever support other replacement policies, the function
 *		should switch on the current policy and act accordingly.
 *
 * Return:      N/A
 *
 * Programmer:  John Mainzer, 5/19/04
 *
 * Modifications:
 *
 *		JRM - 7/27/04
 *		Converted the function H5C_update_rp_for_unprotect() to
 *		the macro H5C__UPDATE_RP_FOR_UNPROTECT in an effort to
 *		squeeze a bit more performance out of the cache.
 *
 *		At least for the first cut, I am leaving the comments and
 *		white space in the macro.  If they cause difficulties with
 *		pre-processor, I'll have to remove them.
 *
 *		JRM - 7/28/04
 *		Split macro into two version, one supporting the clean and
 *		dirty LRU lists, and the other not.  Yet another attempt
 *		at optimization.
 *
 *		JRM - 3/17/06
 *		Modified macro to put pinned entries on the pinned entry
 *		list instead of inserting them in the data structures
 *		maintained by the replacement policy.
 *
 *-------------------------------------------------------------------------
 */

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

#define H5C__UPDATE_RP_FOR_UNPROTECT(cache_ptr, entry_ptr, fail_val)       \
{                                                                          \
    HDassert( (cache_ptr) );                                               \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                    \
    HDassert( (entry_ptr) );                                               \
    HDassert( (entry_ptr)->is_protected);                                  \
    HDassert( (entry_ptr)->size > 0 );                                     \
                                                                           \
    /* Regardless of the replacement policy, remove the entry from the     \
     * protected list.                                                     \
     */                                                                    \
    H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->pl_head_ptr,                 \
                    (cache_ptr)->pl_tail_ptr, (cache_ptr)->pl_len,         \
                    (cache_ptr)->pl_size, (fail_val))                      \
                                                                           \
    if ( (entry_ptr)->is_pinned ) {                                        \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->pel_head_ptr,           \
                         (cache_ptr)->pel_tail_ptr,                        \
                         (cache_ptr)->pel_len,                             \
                         (cache_ptr)->pel_size, (fail_val))                \
                                                                           \
    } else {                                                               \
                                                                           \
        /* modified LRU specific code */                                   \
                                                                           \
        /* insert the entry at the head of the LRU list. */                \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,           \
                         (cache_ptr)->LRU_tail_ptr,                        \
                         (cache_ptr)->LRU_list_len,                        \
                         (cache_ptr)->LRU_list_size, (fail_val))           \
                                                                           \
        /* Similarly, insert the entry at the head of either the clean or  \
         * dirty LRU list as appropriate.                                  \
         */                                                                \
                                                                           \
        if ( (entry_ptr)->is_dirty ) {                                     \
                                                                           \
            H5C__AUX_DLL_PREPEND((entry_ptr), (cache_ptr)->dLRU_head_ptr,  \
                                 (cache_ptr)->dLRU_tail_ptr,               \
                                 (cache_ptr)->dLRU_list_len,               \
                                 (cache_ptr)->dLRU_list_size, (fail_val))  \
                                                                           \
        } else {                                                           \
                                                                           \
            H5C__AUX_DLL_PREPEND((entry_ptr), (cache_ptr)->cLRU_head_ptr,  \
                                 (cache_ptr)->cLRU_tail_ptr,               \
                                 (cache_ptr)->cLRU_list_len,               \
                                 (cache_ptr)->cLRU_list_size, (fail_val))  \
        }                                                                  \
                                                                           \
        /* End modified LRU specific code. */                              \
    }                                                                      \
                                                                           \
} /* H5C__UPDATE_RP_FOR_UNPROTECT */

#else /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#define H5C__UPDATE_RP_FOR_UNPROTECT(cache_ptr, entry_ptr, fail_val)       \
{                                                                          \
    HDassert( (cache_ptr) );                                               \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                    \
    HDassert( (entry_ptr) );                                               \
    HDassert( (entry_ptr)->is_protected);                                  \
    HDassert( (entry_ptr)->size > 0 );                                     \
                                                                           \
    /* Regardless of the replacement policy, remove the entry from the     \
     * protected list.                                                     \
     */                                                                    \
    H5C__DLL_REMOVE((entry_ptr), (cache_ptr)->pl_head_ptr,                 \
                    (cache_ptr)->pl_tail_ptr, (cache_ptr)->pl_len,         \
                    (cache_ptr)->pl_size, (fail_val))                      \
                                                                           \
    if ( (entry_ptr)->is_pinned ) {                                        \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->pel_head_ptr,           \
                         (cache_ptr)->pel_tail_ptr,                        \
                         (cache_ptr)->pel_len,                             \
                         (cache_ptr)->pel_size, (fail_val))                \
                                                                           \
    } else {                                                               \
                                                                           \
        /* modified LRU specific code */                                   \
                                                                           \
        /* insert the entry at the head of the LRU list. */                \
                                                                           \
        H5C__DLL_PREPEND((entry_ptr), (cache_ptr)->LRU_head_ptr,           \
                         (cache_ptr)->LRU_tail_ptr,                        \
                         (cache_ptr)->LRU_list_len,                        \
                         (cache_ptr)->LRU_list_size, (fail_val))           \
                                                                           \
        /* End modified LRU specific code. */                              \
    }                                                                      \
} /* H5C__UPDATE_RP_FOR_UNPROTECT */

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#ifdef H5_HAVE_PARALLEL

#if H5C_DO_SANITY_CHECKS

#define H5C__COLL_DLL_PRE_REMOVE_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv) \
if ( ( (hd_ptr) == NULL ) ||                                                   \
     ( (tail_ptr) == NULL ) ||                                                 \
     ( (entry_ptr) == NULL ) ||                                                \
     ( (len) <= 0 ) ||                                                         \
     ( (Size) < (entry_ptr)->size ) ||                                         \
     ( ( (Size) == (entry_ptr)->size ) && ( ! ( (len) == 1 ) ) ) ||            \
     ( ( (entry_ptr)->coll_prev == NULL ) && ( (hd_ptr) != (entry_ptr) ) ) ||  \
     ( ( (entry_ptr)->coll_next == NULL ) && ( (tail_ptr) != (entry_ptr) ) ) || \
     ( ( (len) == 1 ) &&                                                       \
       ( ! ( ( (hd_ptr) == (entry_ptr) ) && ( (tail_ptr) == (entry_ptr) ) &&   \
             ( (entry_ptr)->coll_next == NULL ) &&                             \
             ( (entry_ptr)->coll_prev == NULL ) &&                             \
             ( (Size) == (entry_ptr)->size )                                   \
           )                                                                   \
       )                                                                       \
     )                                                                         \
   ) {                                                                         \
    HDassert(0 && "coll DLL pre remove SC failed");                            \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "coll DLL pre remove SC failed")  \
}

#define H5C__COLL_DLL_SC(head_ptr, tail_ptr, len, Size, fv)                 \
if ( ( ( ( (head_ptr) == NULL ) || ( (tail_ptr) == NULL ) ) &&              \
       ( (head_ptr) != (tail_ptr) )                                         \
     ) ||                                                                   \
     ( (len) < 0 ) ||                                                       \
     ( (Size) < 0 ) ||                                                      \
     ( ( (len) == 1 ) &&                                                    \
       ( ( (head_ptr) != (tail_ptr) ) || ( (Size) <= 0 ) ||                 \
         ( (head_ptr) == NULL ) || ( (head_ptr)->size != (Size) )           \
       )                                                                    \
     ) ||                                                                   \
     ( ( (len) >= 1 ) &&                                                    \
       ( ( (head_ptr) == NULL ) || ( (head_ptr)->coll_prev != NULL ) ||     \
         ( (tail_ptr) == NULL ) || ( (tail_ptr)->coll_next != NULL )        \
       )                                                                    \
     )                                                                      \
   ) {                                                                      \
    HDassert(0 && "COLL DLL sanity check failed");                          \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "COLL DLL sanity check failed") \
}

#define H5C__COLL_DLL_PRE_INSERT_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv) \
if ( ( (entry_ptr) == NULL ) ||                                                \
     ( (entry_ptr)->coll_next != NULL ) ||                                     \
     ( (entry_ptr)->coll_prev != NULL ) ||                                     \
     ( ( ( (hd_ptr) == NULL ) || ( (tail_ptr) == NULL ) ) &&                   \
       ( (hd_ptr) != (tail_ptr) )                                              \
     ) ||                                                                      \
     ( ( (len) == 1 ) &&                                                       \
       ( ( (hd_ptr) != (tail_ptr) ) || ( (Size) <= 0 ) ||                      \
         ( (hd_ptr) == NULL ) || ( (hd_ptr)->size != (Size) )                  \
       )                                                                       \
     ) ||                                                                      \
     ( ( (len) >= 1 ) &&                                                       \
       ( ( (hd_ptr) == NULL ) || ( (hd_ptr)->coll_prev != NULL ) ||            \
         ( (tail_ptr) == NULL ) || ( (tail_ptr)->coll_next != NULL )           \
       )                                                                       \
     )                                                                         \
   ) {                                                                         \
    HDassert(0 && "COLL DLL pre insert SC failed");                            \
    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, (fv), "COLL DLL pre insert SC failed")  \
}

#else /* H5C_DO_SANITY_CHECKS */

#define H5C__COLL_DLL_PRE_REMOVE_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv)
#define H5C__COLL_DLL_SC(head_ptr, tail_ptr, len, Size, fv)
#define H5C__COLL_DLL_PRE_INSERT_SC(entry_ptr, hd_ptr, tail_ptr, len, Size, fv)

#endif /* H5C_DO_SANITY_CHECKS */


#define H5C__COLL_DLL_APPEND(entry_ptr, head_ptr, tail_ptr, len, Size, fail_val) \
{                                                                            \
    H5C__COLL_DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size,    \
                               fail_val)                                     \
    if ( (head_ptr) == NULL )                                                \
    {                                                                        \
       (head_ptr) = (entry_ptr);                                             \
       (tail_ptr) = (entry_ptr);                                             \
    }                                                                        \
    else                                                                     \
    {                                                                        \
       (tail_ptr)->coll_next = (entry_ptr);                                  \
       (entry_ptr)->coll_prev = (tail_ptr);                                  \
       (tail_ptr) = (entry_ptr);                                             \
    }                                                                        \
    (len)++;                                                                 \
    (Size) += entry_ptr->size;                                               \
} /* H5C__COLL_DLL_APPEND() */

#define H5C__COLL_DLL_PREPEND(entry_ptr, head_ptr, tail_ptr, len, Size, fv)  \
{                                                                            \
    H5C__COLL_DLL_PRE_INSERT_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv)\
    if ( (head_ptr) == NULL )                                                \
    {                                                                        \
       (head_ptr) = (entry_ptr);                                             \
       (tail_ptr) = (entry_ptr);                                             \
    }                                                                        \
    else                                                                     \
    {                                                                        \
       (head_ptr)->coll_prev = (entry_ptr);                                  \
       (entry_ptr)->coll_next = (head_ptr);                                  \
       (head_ptr) = (entry_ptr);                                             \
    }                                                                        \
    (len)++;                                                                 \
    (Size) += entry_ptr->size;                                               \
} /* H5C__COLL_DLL_PREPEND() */

#define H5C__COLL_DLL_REMOVE(entry_ptr, head_ptr, tail_ptr, len, Size, fv)   \
{                                                                            \
    H5C__COLL_DLL_PRE_REMOVE_SC(entry_ptr, head_ptr, tail_ptr, len, Size, fv)\
    {                                                                        \
       if ( (head_ptr) == (entry_ptr) )                                      \
       {                                                                     \
          (head_ptr) = (entry_ptr)->coll_next;                               \
          if ( (head_ptr) != NULL )                                          \
             (head_ptr)->coll_prev = NULL;                                   \
       }                                                                     \
       else                                                                  \
       {                                                                     \
          (entry_ptr)->coll_prev->coll_next = (entry_ptr)->coll_next;        \
       }                                                                     \
       if ( (tail_ptr) == (entry_ptr) )                                      \
       {                                                                     \
          (tail_ptr) = (entry_ptr)->coll_prev;                               \
          if ( (tail_ptr) != NULL )                                          \
             (tail_ptr)->coll_next = NULL;                                   \
       }                                                                     \
       else                                                                  \
          (entry_ptr)->coll_next->coll_prev = (entry_ptr)->coll_prev;        \
       entry_ptr->coll_next = NULL;                                          \
       entry_ptr->coll_prev = NULL;                                          \
       (len)--;                                                              \
       (Size) -= entry_ptr->size;                                            \
    }                                                                        \
} /* H5C__COLL_DLL_REMOVE() */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__INSERT_IN_COLL_LIST
 *
 * Purpose:     Insert entry into collective entries list
 *
 * Return:      N/A
 *
 * Programmer:  Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */

#define H5C__INSERT_IN_COLL_LIST(cache_ptr, entry_ptr, fail_val)        \
{                                                                       \
    HDassert( (cache_ptr) );                                            \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                 \
    HDassert( (entry_ptr) );                                            \
                                                                        \
    /* insert the entry at the head of the list. */                     \
                                                                        \
    H5C__COLL_DLL_PREPEND((entry_ptr), (cache_ptr)->coll_head_ptr,      \
                          (cache_ptr)->coll_tail_ptr,                   \
                          (cache_ptr)->coll_list_len,                   \
                          (cache_ptr)->coll_list_size,                  \
                          (fail_val))                                   \
                                                                        \
} /* H5C__INSERT_IN_COLL_LIST */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__REMOVE_FROM_COLL_LIST
 *
 * Purpose:     Remove entry from collective entries list
 *
 * Return:      N/A
 *
 * Programmer:  Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */

#define H5C__REMOVE_FROM_COLL_LIST(cache_ptr, entry_ptr, fail_val)      \
{                                                                       \
    HDassert( (cache_ptr) );                                            \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                 \
    HDassert( (entry_ptr) );                                            \
                                                                        \
    /* remove the entry from the list. */                               \
                                                                        \
    H5C__COLL_DLL_REMOVE((entry_ptr), (cache_ptr)->coll_head_ptr,       \
                         (cache_ptr)->coll_tail_ptr,                    \
                         (cache_ptr)->coll_list_len,                    \
                         (cache_ptr)->coll_list_size,                   \
                         (fail_val))                                    \
                                                                        \
} /* H5C__REMOVE_FROM_COLL_LIST */


/*-------------------------------------------------------------------------
 *
 * Macro:	H5C__MOVE_TO_TOP_IN_COLL_LIST
 *
 * Purpose:     Update entry position in collective entries list
 *
 * Return:      N/A
 *
 * Programmer:  Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */

#define H5C__MOVE_TO_TOP_IN_COLL_LIST(cache_ptr, entry_ptr, fail_val)   \
{                                                                       \
    HDassert( (cache_ptr) );                                            \
    HDassert( (cache_ptr)->magic == H5C__H5C_T_MAGIC );                 \
    HDassert( (entry_ptr) );                                            \
                                                                        \
    /* Remove entry and insert at the head of the list. */              \
    H5C__COLL_DLL_REMOVE((entry_ptr), (cache_ptr)->coll_head_ptr,       \
                         (cache_ptr)->coll_tail_ptr,                    \
                         (cache_ptr)->coll_list_len,                    \
                         (cache_ptr)->coll_list_size,                   \
                         (fail_val))                                    \
                                                                        \
    H5C__COLL_DLL_PREPEND((entry_ptr), (cache_ptr)->coll_head_ptr,      \
                          (cache_ptr)->coll_tail_ptr,                   \
                          (cache_ptr)->coll_list_len,                   \
                          (cache_ptr)->coll_list_size,                  \
                          (fail_val))                                   \
                                                                        \
} /* H5C__MOVE_TO_TOP_IN_COLL_LIST */
#endif /* H5_HAVE_PARALLEL */


/****************************/
/* Package Private Typedefs */
/****************************/

/****************************************************************************
 *
 * structure H5C_tag_info_t
 *
 * Structure about each set of tagged entries for an object in the file.
 *
 * Each H5C_tag_info_t struct corresponds to a particular object in the file.
 *
 * Each H5C_cache_entry struct in the linked list of entries for this tag
 *      also contains a pointer back to the H5C_tag_info_t struct for the
 *      overall object.
 *
 *
 * The fields of this structure are discussed individually below:
 *
 * tag:	Address (i.e. "tag") of the object header for all the entries
 *              corresponding to parts of that object.
 *
 * head: Head of doubly-linked list of all entries belonging to the tag.
 *
 * entry_cnt: Number of entries on linked list of entries for this tag.
 *
 * corked: Boolean flag indicating whether entries for this object can be
 * 		evicted.
 *
 ****************************************************************************/
typedef struct H5C_tag_info_t {
    haddr_t tag;                /* Tag (address) of the entries (must be first, for skiplist) */
    H5C_cache_entry_t *head;    /* Head of the list of entries for this tag */
    size_t entry_cnt;           /* Number of entries on list */
    hbool_t corked;             /* Whether this object is corked */
} H5C_tag_info_t;


/****************************************************************************
 *
 * structure H5C_t
 *
 * Catchall structure for all variables specific to an instance of the cache.
 *
 * While the individual fields of the structure are discussed below, the
 * following overview may be helpful.
 *
 * Entries in the cache are stored in an instance of H5TB_TREE, indexed on
 * the entry's disk address.  While the H5TB_TREE is less efficient than
 * hash table, it keeps the entries in address sorted order.  As flushes
 * in parallel mode are more efficient if they are issued in increasing
 * address order, this is a significant benefit.  Also the H5TB_TREE code
 * was readily available, which reduced development time.
 *
 * While the cache was designed with multiple replacement policies in mind,
 * at present only a modified form of LRU is supported.
 *
 *                                              JRM - 4/26/04
 *
 * Profiling has indicated that searches in the instance of H5TB_TREE are
 * too expensive.  To deal with this issue, I have augmented the cache
 * with a hash table in which all entries will be stored.  Given the
 * advantages of flushing entries in increasing address order, the TBBT
 * is retained, but only dirty entries are stored in it.  At least for
 * now, we will leave entries in the TBBT after they are flushed.
 *
 * Note that index_size and index_len now refer to the total size of
 * and number of entries in the hash table.
 *
 *						JRM - 7/19/04
 *
 * The TBBT has since been replaced with a skip list.  This change
 * greatly predates this note.
 *
 *						JRM - 9/26/05
 *
 * magic:	Unsigned 32 bit integer always set to H5C__H5C_T_MAGIC. 
 * 		This field is used to validate pointers to instances of
 * 		H5C_t.
 *
 * flush_in_progress: Boolean flag indicating whether a flush is in
 * 		progress.
 *
 * trace_file_ptr:  File pointer pointing to the trace file, which is used
 *              to record cache operations for use in simulations and design
 *              studies.  This field will usually be NULL, indicating that
 *              no trace file should be recorded.
 *
 *              Since much of the code supporting the parallel metadata
 *              cache is in H5AC, we don't write the trace file from
 *              H5C.  Instead, H5AC reads the trace_file_ptr as needed.
 *
 *              When we get to using H5C in other places, we may add
 *              code to write trace file data at the H5C level as well.
 *
 * logging_enabled: Boolean flag indicating whether cache logging
 *              which is used to record cache operations for use in
 *              debugging and performance analysis. When this flag is set
 *              to TRUE, it means that the log file is open and ready to
 *              receive log entries. It does NOT mean that cache operations
 *              are currently being recorded. That is controlled by the
 *              currently_logging flag (below).
 *
 *              Since much of the code supporting the parallel metadata
 *              cache is in H5AC, we don't write the trace file from
 *              H5C.  Instead, H5AC reads the trace_file_ptr as needed.
 *
 *              When we get to using H5C in other places, we may add
 *              code to write trace file data at the H5C level as well.
 *
 * currently_logging: Boolean flag that indicates if cache operations are
 *              currently being logged. This flag is flipped by the
 *              H5Fstart/stop_mdc_logging functions.
 *
 * log_file_ptr:  File pointer pointing to the log file. The I/O functions
 *              in stdio.h are used to write to the log file regardless of
 *              the VFD selected.
 *
 * aux_ptr:	Pointer to void used to allow wrapper code to associate
 *		its data with an instance of H5C_t.  The H5C cache code
 *		sets this field to NULL, and otherwise leaves it alone.
 *
 * max_type_id:	Integer field containing the maximum type id number assigned
 *		to a type of entry in the cache.  All type ids from 0 to
 *		max_type_id inclusive must be defined.  The names of the
 *		types are stored in the type_name_table discussed below, and
 *		indexed by the ids.
 *
 * class_table_ptr: Pointer to an array of H5C_class_t of length
 *              max_type_id + 1.  Entry classes for the cache.
 *
 * max_cache_size:  Nominal maximum number of bytes that may be stored in the
 *              cache.  This value should be viewed as a soft limit, as the
 *              cache can exceed this value under the following circumstances:
 *
 *              a) All entries in the cache are protected, and the cache is
 *                 asked to insert a new entry.  In this case the new entry
 *                 will be created.  If this causes the cache to exceed
 *                 max_cache_size, it will do so.  The cache will attempt
 *                 to reduce its size as entries are unprotected.
 *
 *              b) When running in parallel mode, the cache may not be
 *		   permitted to flush a dirty entry in response to a read.
 *		   If there are no clean entries available to evict, the
 *		   cache will exceed its maximum size.  Again the cache
 *                 will attempt to reduce its size to the max_cache_size
 *                 limit on the next cache write.
 *
 *		c) When an entry increases in size, the cache may exceed
 *		   the max_cache_size limit until the next time the cache
 *		   attempts to load or insert an entry.
 *
 *		d) When the evictions_enabled field is false (see below),
 *		   the cache size will increase without limit until the
 *		   field is set to true.
 *
 * min_clean_size: Nominal minimum number of clean bytes in the cache.
 *              The cache attempts to maintain this number of bytes of
 *              clean data so as to avoid case b) above.  Again, this is
 *              a soft limit.
 *
 * close_warning_received: Boolean flag indicating that a file closing 
 *		warning has been received.
 *
 *
 * In addition to the call back functions required for each entry, the
 * cache requires the following call back functions for this instance of
 * the cache as a whole:
 *
 * check_write_permitted:  In certain applications, the cache may not
 *		be allowed to write to disk at certain time.  If specified,
 *		the check_write_permitted function is used to determine if
 *		a write is permissible at any given point in time.
 *
 *		If no such function is specified (i.e. this field is NULL),
 *		the cache uses the following write_permitted field to
 *		determine whether writes are permitted.
 *
 * write_permitted: If check_write_permitted is NULL, this boolean flag
 *		indicates whether writes are permitted.
 *
 * log_flush:	If provided, this function is called whenever a dirty
 *		entry is flushed to disk.
 *
 *
 * In cases where memory is plentiful, and performance is an issue, it may
 * be useful to disable all cache evictions, and thereby postpone metadata
 * writes.  The following field is used to implement this.
 *
 * evictions_enabled:  Boolean flag that is initialized to TRUE.  When
 * 		this flag is set to FALSE, the metadata cache will not
 * 		attempt to evict entries to make space for newly protected
 * 		entries, and instead the will grow without limit.
 * 		
 * 		Needless to say, this feature must be used with care.
 *
 *
 * The cache requires an index to facilitate searching for entries.  The
 * following fields support that index.
 *
 * Addendum:  JRM -- 10/14/15
 *
 * We sometimes need to visit all entries in the cache.  In the past, this
 * was done by scanning the hash table.  However, this is expensive, and 
 * we have come to scan the hash table often enough that it has become a 
 * performance issue.  To repair this, I have added code to maintain a 
 * list of all entries in the index -- call this list the index list.  
 *
 * The index list is maintained by the same macros that maintain the 
 * index, and must have the same length and size as the index proper.
 *
 * index_len:   Number of entries currently in the hash table used to index
 *		the cache.
 *
 * index_size:  Number of bytes of cache entries currently stored in the
 *              hash table used to index the cache.
 *
 *              This value should not be mistaken for footprint of the
 *              cache in memory.  The average cache entry is small, and
 *              the cache has a considerable overhead.  Multiplying the
 *              index_size by three should yield a conservative estimate
 *              of the cache's memory footprint.
 *
 * index_ring_len: Array of integer of length H5C_RING_NTYPES used to 
 *		maintain a count of entries in the index by ring.  Note 
 *		that the sum of all the cells in this array must equal 
 *		the value stored in index_len above.
 *
 * index_ring_size: Array of size_t of length H5C_RING_NTYPES used to 
 *		maintain the sum of the sizes of all entries in the index
 *		by ring.  Note that the sum of all cells in this array must
 *		equal the value stored in index_size above.
 *
 * clean_index_size: Number of bytes of clean entries currently stored in
 * 		the hash table.  Note that the index_size field (above)
 *		is also the sum of the sizes of all entries in the cache.
 *		Thus we should have the invariant that clean_index_size +
 *		dirty_index_size == index_size.
 *
 *		WARNING:
 *
 *		   The value of the clean_index_size must not be mistaken
 *		   for the current clean size of the cache.  Rather, the
 *		   clean size of the cache is the current value of
 *		   clean_index_size plus the amount of empty space (if any)
 *                 in the cache.
 *
 * clean_index_ring_size: Array of size_t of length H5C_RING_NTYPES used to
 *		maintain the sum of the sizes of all clean entries in the 
 *		index by ring.  Note that the sum of all cells in this array 
 *		must equal the value stored in clean_index_size above.
 *
 * dirty_index_size: Number of bytes of dirty entries currently stored in
 * 		the hash table.  Note that the index_size field (above)
 *		is also the sum of the sizes of all entries in the cache.
 *		Thus we should have the invariant that clean_index_size +
 *		dirty_index_size == index_size.
 *
 * dirty_index_ring_size: Array of size_t of length H5C_RING_NTYPES used to
 *		maintain the sum of the sizes of all dirty entries in the 
 *		index by ring.  Note that the sum of all cells in this array 
 *		must equal the value stored in dirty_index_size above.
 *
 * index:	Array of pointer to H5C_cache_entry_t of size
 *		H5C__HASH_TABLE_LEN.  At present, this value is a power
 *		of two, not the usual prime number.
 *
 *		I hope that the variable size of cache elements, the large
 *		hash table size, and the way in which HDF5 allocates space
 *		will combine to avoid problems with periodicity.  If so, we
 *		can use a trivial hash function (a bit-and and a 3 bit left
 *		shift) with some small savings.
 *
 *		If not, it will become evident in the statistics. Changing
 *		to the usual prime number length hash table will require
 *		changing the H5C__HASH_FCN macro and the deletion of the
 *		H5C__HASH_MASK #define.  No other changes should be required.
 *
 * il_len:	Number of entries on the index list.  
 *
 *		This must always be equal to index_len.  As such, this 
 *		field is redundant.  However, the existing linked list 
 *		management macros expect to maintain a length field, so 
 *		this field exists primarily to avoid adding complexity to
 *		these macros.
 *
 * il_size: 	Number of bytes of cache entries currently stored in the
 *		index list.
 *
 *		This must always be equal to index_size.  As such, this 
 *		field is redundant.  However, the existing linked list 
 *		management macros expect to maintain a size field, so 
 *		this field exists primarily to avoid adding complexity to
 *		these macros.
 *
 * il_head:	Pointer to the head of the doubly linked list of entries in
 *              the index list.  Note that cache entries on this list are 
 *		linked by their il_next and il_prev fields.
 *
 *              This field is NULL if the index is empty.
 *
 * il_tail:	Pointer to the tail of the doubly linked list of entries in
 *              the index list.  Note that cache entries on this list are 
 *              linked by their il_next and il_prev fields.
 *
 *              This field is NULL if the index is empty.
 *
 *
 * With the addition of the take ownership flag, it is possible that 
 * an entry may be removed from the cache as the result of the flush of 
 * a second entry.  In general, this causes little trouble, but it is 
 * possible that the entry removed may be the next entry in the scan of 
 * a list.  In this case, we must be able to detect the fact that the 
 * entry has been removed, so that the scan doesn't attempt to proceed with
 * an entry that is no longer in the cache.
 *
 * The following fields are maintained to facilitate this.
 *
 * entries_removed_counter:	Counter that is incremented each time an
 *		entry is removed from the cache by any means (eviction, 
 *		expungement, or take ownership at this point in time).
 *		Functions that perform scans on lists may set this field
 *		to zero prior to calling H5C__flush_single_entry().  
 *		Unexpected changes to the counter indicate that an entry 
 *		was removed from the cache as a side effect of the flush.
 *
 * last_entry_removed_ptr:	Pointer to the instance of H5C_cache_entry_t
 *		which contained the last entry to be removed from the cache,
 *		or NULL if there either is no such entry, or if a function
 *		performing a scan of a list has set this field to NULL prior
 *		to calling H5C__flush_single_entry().
 *
 *		WARNING!!! This field must NEVER be dereferenced.  It is 
 *		maintained to allow functions that perform scans of lists
 *		to compare this pointer with their pointers to next, thus
 *		allowing them to avoid unnecessary restarts of scans if the
 *		pointers don't match, and if entries_removed_counter is 
 *		one.
 *
 * entry_watched_for_removal:	Pointer to an instance of H5C_cache_entry_t
 *		which contains the 'next' entry for an iteration.  Removing
 *              this entry must trigger a rescan of the iteration, so each
 *              entry removed from the cache is compared against this pointer
 *              and the pointer is reset to NULL if the watched entry is removed.
 *              (This functions similarly to a "dead man's switch")
 *
 *
 * When we flush the cache, we need to write entries out in increasing
 * address order.  An instance of a skip list is used to store dirty entries in
 * sorted order.  Whether it is cheaper to sort the dirty entries as needed,
 * or to maintain the list is an open question.  At a guess, it depends
 * on how frequently the cache is flushed.  We will see how it goes.
 *
 * For now at least, I will not remove dirty entries from the list as they
 * are flushed. (this has been changed -- dirty entries are now removed from
 * the skip list as they are flushed.  JRM - 10/25/05)
 *
 * slist_changed: Boolean flag used to indicate whether the contents of 
 *		the slist has changed since the last time this flag was
 *		reset.  This is used in the cache flush code to detect 
 *		conditions in which pre-serialize or serialize callbacks
 *		have modified the slist -- which obliges us to restart 
 *		the scan of the slist from the beginning.
 *
 * slist_len:   Number of entries currently in the skip list
 *              used to maintain a sorted list of dirty entries in the
 *              cache.
 *
 * slist_size:  Number of bytes of cache entries currently stored in the
 *              skip list used to maintain a sorted list of
 *              dirty entries in the cache.
 *
 * slist_ring_len: Array of integer of length H5C_RING_NTYPES used to 
 *		maintain a count of entries in the slist by ring.  Note 
 *		that the sum of all the cells in this array must equal 
 *		the value stored in slist_len above.
 *
 * slist_ring_size: Array of size_t of length H5C_RING_NTYPES used to
 *              maintain the sum of the sizes of all entries in the 
 *		slist by ring.  Note that the sum of all cells in this 
 *		array must equal the value stored in slist_size above.
 *
 * slist_ptr:   pointer to the instance of H5SL_t used maintain a sorted
 *              list of dirty entries in the cache.  This sorted list has
 *              two uses:
 *
 *              a) It allows us to flush dirty entries in increasing address
 *                 order, which results in significant savings.
 *
 *              b) It facilitates checking for adjacent dirty entries when
 *                 attempting to evict entries from the cache.  While we
 *                 don't use this at present, I hope that this will allow
 *                 some optimizations when I get to it.
 *
 * num_last_entries: The number of entries in the cache that can only be
 *		flushed after all other entries in the cache have
 *              been flushed. At this time, this will only ever be
 *              one entry (the superblock), and the code has been
 *              protected with HDasserts to enforce this. This restraint
 *              can certainly be relaxed in the future if the need for
 *              multiple entries being flushed last arises, though
 *              explicit tests for that case should be added when said
 *              HDasserts are removed.
 *
 *		Update: There are now two possible last entries
 *		(superblock and file driver info message).  This
 *		number will probably increase as we add superblock
 *		messages.   JRM -- 11/18/14
 *
 * With the addition of the fractal heap, the cache must now deal with
 * the case in which entries may be dirtied, moved, or have their sizes
 * changed during a flush.  To allow sanity checks in this situation, the
 * following two fields have been added.  They are only compiled in when
 * H5C_DO_SANITY_CHECKS is TRUE.
 *
 * slist_len_increase: Number of entries that have been added to the
 * 		slist since the last time this field was set to zero.
 *		Note that this value can be negative.
 *
 * slist_size_increase: Total size of all entries that have been added
 * 		to the slist since the last time this field was set to
 * 		zero.  Note that this value can be negative.
 *
 * Cache entries belonging to a particular object are "tagged" with that
 * object's base object header address.
 *
 * The following fields are maintained to facilitate this.
 *
 * tag_list: A skip list to track entries that belong to an object.
 *                Each H5C_tag_info_t struct on the tag list corresponds to
 *                a particular object in the file.  Tagged entries can be
 *                flushed or evicted as a group, or corked to prevent entries
 *                from being evicted from the cache.
 *
 *                "Global" entries, like the superblock and the file's
 *                freelist, as well as shared entries like global
 *                heaps and shared object header messages, are not tagged.
 *
 * ignore_tags:	Boolean flag to disable tag validation during entry insertion.
 *
 * When a cache entry is protected, it must be removed from the LRU
 * list(s) as it cannot be either flushed or evicted until it is unprotected.
 * The following fields are used to implement the protected list (pl).
 *
 * pl_len:      Number of entries currently residing on the protected list.
 *
 * pl_size:     Number of bytes of cache entries currently residing on the
 *              protected list.
 *
 * pl_head_ptr: Pointer to the head of the doubly linked list of protected
 *              entries.  Note that cache entries on this list are linked
 *              by their next and prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 * pl_tail_ptr: Pointer to the tail of the doubly linked list of protected
 *              entries.  Note that cache entries on this list are linked
 *              by their next and prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 *
 * For very frequently used entries, the protect/unprotect overhead can
 * become burdensome.  To avoid this overhead, I have modified the cache
 * to allow entries to be "pinned".  A pinned entry is similar to a
 * protected entry, in the sense that it cannot be evicted, and that
 * the entry can be modified at any time.
 *
 * Pinning an entry has the following implications:
 *
 *	1) A pinned entry cannot be evicted.  Thus unprotected
 *         pinned entries reside in the pinned entry list, instead
 *         of the LRU list(s) (or other lists maintained by the current
 *         replacement policy code).
 *
 *      2) A pinned entry can be accessed or modified at any time.
 *         This places an additional burden on the associated pre-serialize
 *	   and serialize callbacks, which must ensure the the entry is in 
 *	   a consistent state before creating an image of it.
 *
 *      3) A pinned entry can be marked as dirty (and possibly
 *         change size) while it is unprotected.
 *
 *      4) The flush-destroy code must allow pinned entries to
 *         be unpinned (and possibly unprotected) during the
 *         flush.
 *
 * Since pinned entries cannot be evicted, they must be kept on a pinned
 * entry list (pel), instead of being entrusted to the replacement policy 
 * code.
 *
 * Maintaining the pinned entry list requires the following fields:
 *
 * pel_len:	Number of entries currently residing on the pinned
 * 		entry list.
 *
 * pel_size:	Number of bytes of cache entries currently residing on
 * 		the pinned entry list.
 *
 * pel_head_ptr: Pointer to the head of the doubly linked list of pinned
 * 		but not protected entries.  Note that cache entries on
 * 		this list are linked by their next and prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 * pel_tail_ptr: Pointer to the tail of the doubly linked list of pinned
 * 		but not protected entries.  Note that cache entries on
 * 		this list are linked by their next and prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 *
 * The cache must have a replacement policy, and the fields supporting this
 * policy must be accessible from this structure.
 *
 * While there has been interest in several replacement policies for
 * this cache, the initial development schedule is tight.  Thus I have
 * elected to support only a modified LRU (least recently used) policy 
 * for the first cut.
 *
 * To further simplify matters, I have simply included the fields needed
 * by the modified LRU in this structure.  When and if we add support for
 * other policies, it will probably be easiest to just add the necessary
 * fields to this structure as well -- we only create one instance of this
 * structure per file, so the overhead is not excessive.
 *
 *
 * Fields supporting the modified LRU policy:
 *
 * See most any OS text for a discussion of the LRU replacement policy.
 *
 * When operating in parallel mode, we must ensure that a read does not
 * cause a write.  If it does, the process will hang, as the write will
 * be collective and the other processes will not know to participate.
 *
 * To deal with this issue, I have modified the usual LRU policy by adding
 * clean and dirty LRU lists to the usual LRU list.  In general, these 
 * lists are only exist in parallel builds.
 *
 * The clean LRU list is simply the regular LRU list with all dirty cache
 * entries removed.
 *
 * Similarly, the dirty LRU list is the regular LRU list with all the clean
 * cache entries removed.
 *
 * When reading in parallel mode, we evict from the clean LRU list only.
 * This implies that we must try to ensure that the clean LRU list is
 * reasonably well stocked at all times.
 *
 * We attempt to do this by trying to flush enough entries on each write
 * to keep the cLRU_list_size >= min_clean_size.
 *
 * Even if we start with a completely clean cache, a sequence of protects
 * without unprotects can empty the clean LRU list.  In this case, the
 * cache must grow temporarily.  At the next sync point, we will attempt to
 * evict enough entries to reduce index_size to less than max_cache_size.
 * While this will usually be possible, all bets are off if enough entries
 * are protected.
 *
 * Discussions of the individual fields used by the modified LRU replacement
 * policy follow:
 *
 * LRU_list_len:  Number of cache entries currently on the LRU list.
 *
 *              Observe that LRU_list_len + pl_len + pel_len must always 
 *		equal index_len.
 *
 * LRU_list_size:  Number of bytes of cache entries currently residing on the
 *              LRU list.
 *
 *              Observe that LRU_list_size + pl_size + pel_size must always 
 *		equal index_size.
 *
 * LRU_head_ptr:  Pointer to the head of the doubly linked LRU list.  Cache
 *              entries on this list are linked by their next and prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 * LRU_tail_ptr:  Pointer to the tail of the doubly linked LRU list.  Cache
 *              entries on this list are linked by their next and prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 * cLRU_list_len: Number of cache entries currently on the clean LRU list.
 *
 *              Observe that cLRU_list_len + dLRU_list_len must always
 *              equal LRU_list_len.
 *
 * cLRU_list_size:  Number of bytes of cache entries currently residing on
 *              the clean LRU list.
 *
 *              Observe that cLRU_list_size + dLRU_list_size must always
 *              equal LRU_list_size.
 *
 * cLRU_head_ptr:  Pointer to the head of the doubly linked clean LRU list.
 *              Cache entries on this list are linked by their aux_next and
 *              aux_prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 * cLRU_tail_ptr:  Pointer to the tail of the doubly linked clean LRU list.
 *              Cache entries on this list are linked by their aux_next and
 *              aux_prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 * dLRU_list_len: Number of cache entries currently on the dirty LRU list.
 *
 *              Observe that cLRU_list_len + dLRU_list_len must always
 *              equal LRU_list_len.
 *
 * dLRU_list_size:  Number of cache entries currently on the dirty LRU list.
 *
 *              Observe that cLRU_list_len + dLRU_list_len must always
 *              equal LRU_list_len.
 *
 * dLRU_head_ptr:  Pointer to the head of the doubly linked dirty LRU list.
 *              Cache entries on this list are linked by their aux_next and
 *              aux_prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 * dLRU_tail_ptr:  Pointer to the tail of the doubly linked dirty LRU list.
 *              Cache entries on this list are linked by their aux_next and
 *              aux_prev fields.
 *
 *              This field is NULL if the list is empty.
 *
 *
 * Automatic cache size adjustment:
 *
 * While the default cache size is adequate for most cases, we can run into
 * cases where the default is too small.  Ideally, we will let the user
 * adjust the cache size as required.  However, this is not possible in all
 * cases.  Thus I have added automatic cache size adjustment code.
 *
 * The configuration for the automatic cache size adjustment is stored in
 * the structure described below:
 *
 * size_increase_possible:  Depending on the configuration data given
 *		in the resize_ctl field, it may or may not be possible
 *		to increase the size of the cache.  Rather than test for
 *		all the ways this can happen, we simply set this flag when
 *		we receive a new configuration.
 *
 * flash_size_increase_possible: Depending on the configuration data given
 *              in the resize_ctl field, it may or may not be possible
 *              for a flash size increase to occur.  We set this flag
 *              whenever we receive a new configuration so as to avoid
 *              repeated calculations.
 *
 * flash_size_increase_threshold: If a flash cache size increase is possible,
 *              this field is used to store the minimum size of a new entry
 *              or size increase needed to trigger a flash cache size
 *              increase.  Note that this field must be updated whenever
 *              the size of the cache is changed.
 *
 * size_decrease_possible:  Depending on the configuration data given
 *              in the resize_ctl field, it may or may not be possible
 *              to decrease the size of the cache.  Rather than test for
 *              all the ways this can happen, we simply set this flag when
 *              we receive a new configuration.
 *
 * resize_enabled:  This is another convenience flag which is set whenever
 *		a new set of values for resize_ctl are provided.  Very
 *		simply,
 *
 *		    resize_enabled = size_increase_possible ||
 *                                   size_decrease_possible;
 *
 * cache_full:	Boolean flag used to keep track of whether the cache is
 *		full, so we can refrain from increasing the size of a
 *		cache which hasn't used up the space allotted to it.
 *
 *		The field is initialized to FALSE, and then set to TRUE
 *		whenever we attempt to make space in the cache.
 *
 * size_decreased:  Boolean flag set to TRUE whenever the maximum cache
 *		size is decreased.  The flag triggers a call to
 *		H5C__make_space_in_cache() on the next call to H5C_protect().
 *
 * resize_in_progress: As the metadata cache has become re-entrant, it is 
 *		possible that a protect may trigger a call to 
 *		H5C__auto_adjust_cache_size(), which may trigger a flush,
 *		which may trigger a protect, which will result in another 
 *		call to H5C__auto_adjust_cache_size().  
 *
 *		The resize_in_progress boolean flag is used to detect this,
 *		and to prevent the infinite recursion that would otherwise
 *		occur.
 *
 *		Note that this issue is not hypothetical -- this field 
 *		was added 12/29/15 to fix a bug exposed in the testing 
 *		of changes to the file driver info superblock extension
 *		management code needed to support rings.
 *
 * msic_in_progress: As the metadata cache has become re-entrant, and as
 *              the free space manager code has become more tightly 
 *              integrated with the metadata cache, it is possible that 
 *              a call to H5C_insert_entry() may trigger a call to 
 *              H5C_make_space_in_cache(), which, via H5C__flush_single_entry()
 *              and client callbacks, may trigger an infinite regression
 *              of calls to H5C_make_space_in_cache().
 *
 *              The msic_in_progress boolean flag is used to detect this,
 *              and prevent the infinite regression that would otherwise
 *              occur.
 *
 *              Note that this is issue is not hypothetical -- this field 
 *              was added 2/16/17 to address this issue when it was 
 *              exposed by modifications to test/fheap.c to cause it to 
 *              use paged allocation.
 *
 * resize_ctl:	Instance of H5C_auto_size_ctl_t containing configuration
 * 		data for automatic cache resizing.
 *
 * epoch_markers_active:  Integer field containing the number of epoch
 *		markers currently in use in the LRU list.  This value
 *		must be in the range [0, H5C__MAX_EPOCH_MARKERS - 1].
 *
 * epoch_marker_active:  Array of boolean of length H5C__MAX_EPOCH_MARKERS.
 *		This array is used to track which epoch markers are currently
 *		in use.
 *
 * epoch_marker_ringbuf:  Array of int of length H5C__MAX_EPOCH_MARKERS + 1.
 *
 *		To manage the epoch marker cache entries, it is necessary
 *		to track their order in the LRU list.  This is done with
 *		epoch_marker_ringbuf.  When markers are inserted at the
 *		head of the LRU list, the index of the marker in the
 *		epoch_markers array is inserted at the tail of the ring
 *		buffer.  When it becomes the epoch_marker_active'th marker
 *		in the LRU list, it will have worked its way to the head
 *		of the ring buffer as well.  This allows us to remove it
 *		without scanning the LRU list if such is required.
 *
 * epoch_marker_ringbuf_first: Integer field containing the index of the
 *		first entry in the ring buffer.
 *
 * epoch_marker_ringbuf_last: Integer field containing the index of the
 *		last entry in the ring buffer.
 *
 * epoch_marker_ringbuf_size: Integer field containing the number of entries
 *		in the ring buffer.
 *
 * epoch_markers:  Array of instances of H5C_cache_entry_t of length
 *		H5C__MAX_EPOCH_MARKERS.  The entries are used as markers
 *		in the LRU list to identify cache entries that haven't
 *		been accessed for some (small) specified number of
 *		epochs.  These entries (if any) can then be evicted and
 *		the cache size reduced -- ideally without evicting any
 *		of the current working set.  Needless to say, the epoch
 *		length and the number of epochs before an unused entry
 *		must be chosen so that all, or almost all, the working
 *		set will be accessed before the limit.
 *
 *		Epoch markers only appear in the LRU list, never in
 *		the index or slist.  While they are of type
 *		H5C__EPOCH_MARKER_TYPE, and have associated class
 *		functions, these functions should never be called.
 *
 *		The addr fields of these instances of H5C_cache_entry_t
 *		are set to the index of the instance in the epoch_markers
 *		array, the size is set to 0, and the type field points
 *		to the constant structure epoch_marker_class defined
 *		in H5C.c.  The next and prev fields are used as usual
 *		to link the entry into the LRU list.
 *
 *		All other fields are unused.
 *
 *
 * Cache hit rate collection fields:
 *
 * We supply the current cache hit rate on request, so we must keep a
 * simple cache hit rate computation regardless of whether statistics
 * collection is enabled.  The following fields support this capability.
 *
 * cache_hits: Number of cache hits since the last time the cache hit
 *	rate statistics were reset.  Note that when automatic cache
 *	re-sizing is enabled, this field will be reset every automatic
 *	resize epoch.
 *
 * cache_accesses: Number of times the cache has been accessed while
 *	since the last since the last time the cache hit rate statistics
 *	were reset.  Note that when automatic cache re-sizing is enabled,
 *	this field will be reset every automatic resize epoch.
 *
 *
 * Metadata cache image management related fields.
 *
 * image_ctl:	Instance of H5C_cache_image_ctl_t containing configuration
 * 		data for generation of a cache image on file close.
 *
 * serialization_in_progress:  Boolean field that is set to TRUE iff
 *		the cache is in the process of being serialized.  This 
 *		field is needed to support the H5C_serialization_in_progress()
 *		call, which is in turn required for sanity checks in some
 *		cache clients.
 *
 * load_image:	Boolean flag indicating that the metadata cache image 
 *		superblock extension message exists and should be 
 *		read, and the image block read and decoded on the next
 *		call to H5C_protect().  
 *
 * image_loaded:  Boolean flag indicating that the metadata cache has 
 *              loaded the metadata cache image as directed by the 
 *              MDC cache image superblock extension message.
 *
 * delete_image: Boolean flag indicating whether the metadata cache image
 *		superblock message should be deleted and the cache image
 *		file space freed after they have been read and decoded.
 *
 *		This flag should be set to TRUE iff the file is opened 
 *		R/W and there is a cache image to be read.
 *
 * image_addr:  haddr_t containing the base address of the on disk 
 *		metadata cache image, or HADDR_UNDEF if that value is 
 *		undefined.  Note that this field is used both in the 
 *		construction and write, and the read and decode of 
 *		metadata cache image blocks.
 *
 * image_len:	hsize_t containing the size of the on disk metadata cache 
 *		image, or zero if that value is undefined.  Note that this 
 *		field is used both in the construction and write, and the 
 *		read and decode of metadata cache image blocks.
 *
 * image_data_len:  size_t containing the number of bytes of data in the 
 *		on disk metadata cache image, or zero if that value is 
 *		undefined.
 *
 *		In most cases, this value is the same as the image_len
 *		above.  It exists to allow for metadata cache image blocks
 *		that are larger than the actual image.  Thus in all 
 *		cases image_data_len <= image_len.
 *
 * To create the metadata cache image, we must first serialize all the
 * entries in the metadata cache.  This is done by a scan of the index.
 * As entries must be serialized in increasing flush dependency height
 * order, we scan the index repeatedly, once for each flush dependency
 * height in increasing order.
 *
 * This operation is complicated by the fact that entries other the the
 * target may be inserted, loaded, relocated, or removed from the cache 
 * (either by eviction or the take ownership flag) as the result of a 
 * pre_serialize or serialize callback.  While entry removals are not 
 * a problem for the scan of the index, insertions, loads, and relocations
 * are.  Hence the entries loaded, inserted, and relocated counters 
 * listed below have been implemented to allow these conditions to be 
 * detected and dealt with by restarting the scan.
 *
 * The serialization operation is further complicated by the fact that 
 * the flush dependency height of a given entry may increase (as the 
 * result of an entry load or insert) or decrease (as the result of an 
 * entry removal -- via either eviction or the take ownership flag).  The
 * entry_fd_height_change_counter field is maintained to allow detection
 * of this condition, and a restart of the scan when it occurs.
 *
 * Note that all these new fields would work just as well as booleans.
 *
 * entries_loaded_counter: Number of entries loaded into the cache 
 *		since the last time this field was reset.
 *
 * entries_inserted_counter: Number of entries inserted into the cache 
 *		since the last time this field was reset.
 *
 * entries relocated_counter: Number of entries whose base address has
 *		been changed since the last time this field was reset.
 *
 * entry_fd_height_change_counter: Number of entries whose flush dependency
 *		height has changed since the last time this field was reset.
 *
 * The following fields are used assemble the cache image prior to 
 * writing it to disk.
 *
 * num_entries_in_image: Unsigned integer field containing the number of entries
 *		to be copied into the metadata cache image.  Note that 
 *		this value will be less than the number of entries in 
 *		the cache, and the superblock and its related entries 
 *		are not written to the metadata cache image.
 *
 * image_entries: Pointer to a dynamically allocated array of instance of
 *		H5C_image_entry_t of length num_entries_in_image, or NULL
 *		if that array does not exist.  This array is used to
 *		assemble entry data to be included in the image, and to 
 *		sort them by flush dependency height and LRU rank.
 * 
 * image_buffer: Pointer to the dynamically allocated buffer of length
 *		image_len in which the metadata cache image is assembled, 
 *		or NULL if that	buffer does not exist.
 *
 *
 * Free Space Manager Related fields:
 *
 * The free space managers must be informed when we are about to close 
 * or flush the file so that they order themselves accordingly.  This used
 * to be done much later in the close process, but with cache image and 
 * page buffering, this is no longer viable, as we must finalize the on 
 * disk image of all metadata much sooner.
 *
 * This is handled by the H5MF_settle_raw_data_fsm() and
 * H5MF_settle_meta_data_FSM() routines.  As these calls are expensive,
 * the following fields are used to track whether the target free space
 * managers are clean.
 *
 * They are also used in sanity checking, as once a free space manager is
 * settled, it should not become unsettled (i.e. be asked to allocate or
 * free file space) either ever (in the case of a file close) or until the
 * flush is complete.
 *
 * rdfsm_settled:  Boolean flag indicating whether the raw data free space
 *		manager is settled -- i.e. whether the correct space has 
 *		been allocated for it in the file.
 *
 *		Note that the name of this field is deceptive.  In the 
 *		multi file case, the flag applies to all free space 
 *		managers that are not involved in allocating space for
 *		free space manager metadata.
 *
 * mdfsm_settled:  Boolean flag indicating whether the meta data free space
 *              manager is settled -- i.e. whether the correct space has
 *              been allocated for it in the file.
 *
 *              Note that the name of this field is deceptive.  In the 
 *              multi file case, the flag applies only to free space 
 *		managers that are involved in allocating space for free 
 *		space managers.
 *
 *
 * Statistics collection fields:
 *
 * When enabled, these fields are used to collect statistics as described
 * below.  The first set are collected only when H5C_COLLECT_CACHE_STATS
 * is true.
 *
 * hits:        Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type id
 *		equal to the array index has been in cache when requested in
 *		the current epoch.
 *
 * misses:      Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type id
 *		equal to the array index has not been in cache when
 *		requested in the current epoch.
 *
 * write_protects:  Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The
 * 		cells are used to record the number of times an entry with
 * 		type id equal to the array index has been write protected
 * 		in the current epoch.
 *
 * 		Observe that (hits + misses) = (write_protects + read_protects).
 *
 * read_protects: Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The
 * 		cells are used to record the number of times an entry with
 * 		type id equal to the array index has been read protected in
 * 		the current epoch.
 *
 *              Observe that (hits + misses) = (write_protects + read_protects).
 *
 * max_read_protects:  Array of int32 of length H5C__MAX_NUM_TYPE_IDS + 1.
 * 		The cells are used to maximum number of simultaneous read
 * 		protects on any entry with type id equal to the array index
 * 		in the current epoch.
 *
 * insertions:  Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type
 *		id equal to the array index has been inserted into the
 *		cache in the current epoch.
 *
 * pinned_insertions:  Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1. 
 * 		The cells are used to record the number of times an entry
 * 		with type id equal to the array index has been inserted
 * 		pinned into the cache in the current epoch.
 *
 * clears:      Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times a dirty entry with type
 *		id equal to the array index has been cleared in the current
 *		epoch.
 *
 * flushes:     Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type id
 *		equal to the array index has been written to disk in the
 *              current epoch.
 *
 * evictions:   Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type id
 *		equal to the array index has been evicted from the cache in
 *		the current epoch.
 *
 * take_ownerships: Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The 
 *		cells are used to record the number of times an entry with 
 *		type id equal to the array index has been removed from the 
 *		cache via the H5C__TAKE_OWNERSHIP_FLAG in the current epoch.
 *
 * moves:       Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type
 *		id equal to the array index has been moved in the current
 *		epoch.
 *
 * entry_flush_moves: Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1. 
 * 		The cells are used to record the number of times an entry
 * 		with type id equal to the array index has been moved
 * 		during its pre-serialize callback in the current epoch.
 *
 * cache_flush_moves: Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1. 
 * 		The cells are used to record the number of times an entry
 * 		with type id equal to the array index has been moved
 * 		during a cache flush in the current epoch.
 *
 * pins:        Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type
 *		id equal to the array index has been pinned in the current
 *		epoch.
 *
 * unpins:      Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type
 *		id equal to the array index has been unpinned in the current
 *		epoch.
 *
 * dirty_pins:	Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the number of times an entry with type
 *		id equal to the array index has been marked dirty while pinned
 *		in the current epoch.
 *
 * pinned_flushes:  Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The
 * 		cells are used to record the number of times an  entry
 * 		with type id equal to the array index has been flushed while
 * 		pinned in the current epoch.
 *
 * pinned_clears:  Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.  The
 * 		cells are used to record the number of times an  entry
 * 		with type id equal to the array index has been cleared while
 * 		pinned in the current epoch.
 *
 * size_increases:  Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.
 *		The cells are used to record the number of times an entry
 *		with type id equal to the array index has increased in
 *		size in the current epoch.
 *
 * size_decreases:  Array of int64 of length H5C__MAX_NUM_TYPE_IDS + 1.
 *		The cells are used to record the number of times an entry
 *		with type id equal to the array index has decreased in
 *		size in the current epoch.
 *
 * entry_flush_size_changes:  Array of int64 of length
 * 		H5C__MAX_NUM_TYPE_IDS + 1.  The cells are used to record
 * 		the number of times an entry with type id equal to the
 * 		array index has changed size while in its pre-serialize 
 *		callback.
 *
 * cache_flush_size_changes:  Array of int64 of length
 * 		H5C__MAX_NUM_TYPE_IDS + 1.  The cells are used to record
 * 		the number of times an entry with type id equal to the
 * 		array index has changed size during a cache flush
 *
 * total_ht_insertions: Number of times entries have been inserted into the
 *		hash table in the current epoch.
 *
 * total_ht_deletions: Number of times entries have been deleted from the
 *              hash table in the current epoch.
 *
 * successful_ht_searches: int64 containing the total number of successful
 *		searches of the hash table in the current epoch.
 *
 * total_successful_ht_search_depth: int64 containing the total number of
 *		entries other than the targets examined in successful
 *		searches of the hash table in the current epoch.
 *
 * failed_ht_searches: int64 containing the total number of unsuccessful
 *              searches of the hash table in the current epoch.
 *
 * total_failed_ht_search_depth: int64 containing the total number of
 *              entries examined in unsuccessful searches of the hash
 *		table in the current epoch.
 *
 * max_index_len:  Largest value attained by the index_len field in the
 *              current epoch.
 *
 * max_index_size:  Largest value attained by the index_size field in the
 *              current epoch.
 *
 * max_clean_index_size: Largest value attained by the clean_index_size field
 * 		in the current epoch.
 *
 * max_dirty_index_size: Largest value attained by the dirty_index_size field
 * 		in the current epoch.
 *
 * max_slist_len:  Largest value attained by the slist_len field in the
 *              current epoch.
 *
 * max_slist_size:  Largest value attained by the slist_size field in the
 *              current epoch.
 *
 * max_pl_len:  Largest value attained by the pl_len field in the
 *              current epoch.
 *
 * max_pl_size: Largest value attained by the pl_size field in the
 *              current epoch.
 *
 * max_pel_len: Largest value attained by the pel_len field in the
 *              current epoch.
 *
 * max_pel_size: Largest value attained by the pel_size field in the
 *              current epoch.
 *
 * calls_to_msic: Total number of calls to H5C__make_space_in_cache
 *
 * total_entries_skipped_in_msic: Number of clean entries skipped while
 *              enforcing the min_clean_fraction in H5C__make_space_in_cache().
 *
 * total_dirty_pf_entries_skipped_in_msic: Number of dirty prefetched entries
 *              skipped in H5C__make_space_in_cache().  Note that this can 
 *              only occur when a file is opened R/O with a cache image
 *              containing dirty entries.
 *
 * total_entries_scanned_in_msic: Number of clean entries skipped while
 *              enforcing the min_clean_fraction in H5C__make_space_in_cache().
 *
 * max_entries_skipped_in_msic: Maximum number of clean entries skipped
 *              in any one call to H5C__make_space_in_cache().
 *
 * max_dirty_pf_entries_skipped_in_msic: Maximum number of dirty prefetched
 *              entries skipped in any one call to H5C__make_space_in_cache().
 *              Note that this can only occur when the file is opened 
 *              R/O with a cache image containing dirty entries.
 *
 * max_entries_scanned_in_msic: Maximum number of entries scanned over
 *              in any one call to H5C__make_space_in_cache().
 *
 * entries_scanned_to_make_space: Number of entries scanned only when looking
 *              for entries to evict in order to make space in cache.
 *
 *
 * The following fields track statistics on cache images.  
 *
 * images_created:  Integer field containing the number of cache images
 *		created since the last time statistics were reset.  
 *
 *		At present, this field must always be either 0 or 1.
 *		Further, since cache images are only created at file 
 *		close, this field should only be set at that time.
 *
 * images_read: Integer field containing the number of cache images 
 *              read from file.  Note that reading an image is different
 *              from loading it -- reading the image means just that,
 *              while loading the image refers to decoding it and loading
 *              it into the metadata cache.
 *
 *              In the serial case, image_read should always equal 
 *              images_loaded.  However, in the parallel case, the 
 *              image should only be read by process 0.  All other 
 *              processes should receive the cache image via a broadcast
 *              from process 0.
 *
 * images_loaded:  Integer field containing the number of cache images
 *		loaded since the last time statistics were reset.
 *
 *		At present, this field must always be either 0 or 1.
 *		Further, since cache images are only loaded at the 
 *		time of the first protect or on file close, this value
 *		should only change on those events.
 *
 * last_image_size:  Size of the most recently loaded metadata cache image
 *             loaded into the cache, or zero if no image has been
 *             loaded.  
 *
 *             At present, at most one cache image can be loaded into 
 *             the metadata cache for any given file, and this image
 *             will be loaded either on the first protect, or on file
 *             close if no entry is protected before then.
 *
 *
 * Fields for tracking prefetched entries.  Note that flushes and evictions
 * of prefetched entries are tracked in the flushes and evictions arrays 
 * discused above.
 *
 * prefetches:	Number of prefetched entries that are loaded to the 
 *		cache.
 *
 * dirty_prefetches:  Number of dirty prefetched entries that are loaded
 *		into the cache.
 *
 * prefetch_hits:  Number of prefetched entries that are actually used.
 *
 * 
 * As entries are now capable of moving, loading, dirtying, and deleting 
 * other entries in their pre_serialize and serialize callbacks, it has 
 * been necessary to insert code to restart scans of lists so as to avoid 
 * improper behavior if the next entry in the list is the target of one on 
 * these operations.
 *
 * The following fields are use to count such occurrences.  They are used 
 * both in tests (to verify that the scan has been restarted), and to 
 * obtain estimates of how frequently these restarts occur.
 *
 * slist_scan_restarts: Number of times a scan of the slist (that contains
 *		calls to H5C__flush_single_entry()) has been restarted to 
 *		avoid potential issues with change of status of the next 
 *		entry in the scan.
 *
 * LRU_scan_restarts: Number of times a scan of the LRU list (that contains
 *              calls to H5C__flush_single_entry()) has been restarted to 
 *              avoid potential issues with change of status of the next 
 *              entry in the scan.
 *
 * index_scan_restarts: Number of times a scan of the index has been 
 *		restarted to avoid potential issues with load, insertion
 *		or change in flush dependency height of an entry other 
 *		than the target entry as the result of call(s) to the
 *		pre_serialize or serialize callbacks.
 *
 *		Note that at present, this condition can only be triggered
 *		by a call to H5C_serialize_single_entry().
 *
 * The remaining stats are collected only when both H5C_COLLECT_CACHE_STATS
 * and H5C_COLLECT_CACHE_ENTRY_STATS are true.
 *
 * max_accesses: Array of int32 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the maximum number of times any single
 *		entry with type id equal to the array index has been
 *		accessed in the current epoch.
 *
 * min_accesses: Array of int32 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the minimum number of times any single
 *		entry with type id equal to the array index has been
 *		accessed in the current epoch.
 *
 * max_clears:  Array of int32 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the maximum number of times any single
 *		entry with type id equal to the array index has been cleared
 *		in the current epoch.
 *
 * max_flushes: Array of int32 of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *		are used to record the maximum number of times any single
 *		entry with type id equal to the array index has been
 *		flushed in the current epoch.
 *
 * max_size:	Array of size_t of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *              are used to record the maximum size of any single entry
 *		with type id equal to the array index that has resided in
 *		the cache in the current epoch.
 *
 * max_pins:	Array of size_t of length H5C__MAX_NUM_TYPE_IDS + 1.  The cells
 *              are used to record the maximum number of times that any single
 *              entry with type id equal to the array index that has been
 *              marked as pinned in the cache in the current epoch.
 *
 *
 * Fields supporting testing:
 *
 * prefix	Array of char used to prefix debugging output.  The
 *		field is intended to allow marking of output of with
 *		the processes mpi rank.
 *
 * get_entry_ptr_from_addr_counter: Counter used to track the number of
 *              times the H5C_get_entry_ptr_from_addr() function has been
 *              called successfully.  This field is only defined when
 *              NDEBUG is not #defined.
 *
 ****************************************************************************/
struct H5C_t {
    uint32_t			magic;
    hbool_t			flush_in_progress;
    FILE *			trace_file_ptr;
    hbool_t                     logging_enabled;
    hbool_t                     currently_logging;
    FILE *			log_file_ptr;
    void *			aux_ptr;
    int32_t			max_type_id;
    const H5C_class_t * const   *class_table_ptr;
    size_t                      max_cache_size;
    size_t                      min_clean_size;
    H5C_write_permitted_func_t	check_write_permitted;
    hbool_t			write_permitted;
    H5C_log_flush_func_t	log_flush;
    hbool_t			evictions_enabled;
    hbool_t			close_warning_received;

    /* Fields for maintaining [hash table] index of entries */
    uint32_t                    index_len;
    size_t                      index_size;
    uint32_t			index_ring_len[H5C_RING_NTYPES];
    size_t			index_ring_size[H5C_RING_NTYPES];
    size_t 			clean_index_size;
    size_t			clean_index_ring_size[H5C_RING_NTYPES];
    size_t			dirty_index_size;
    size_t			dirty_index_ring_size[H5C_RING_NTYPES];
    H5C_cache_entry_t *	        index[H5C__HASH_TABLE_LEN];
    uint32_t                    il_len;
    size_t                      il_size;
    H5C_cache_entry_t *	        il_head;
    H5C_cache_entry_t *	        il_tail;

    /* Fields to detect entries removed during scans */
    int64_t			entries_removed_counter;
    H5C_cache_entry_t *		last_entry_removed_ptr;
    H5C_cache_entry_t *		entry_watched_for_removal;

    /* Fields for maintaining list of in-order entries, for flushing */
    hbool_t			slist_changed;
    uint32_t                    slist_len;
    size_t                      slist_size;
    uint32_t			slist_ring_len[H5C_RING_NTYPES];
    size_t			slist_ring_size[H5C_RING_NTYPES];
    H5SL_t *                    slist_ptr;
    uint32_t                    num_last_entries;
#if H5C_DO_SANITY_CHECKS
    int32_t			slist_len_increase;
    ssize_t			slist_size_increase;
#endif /* H5C_DO_SANITY_CHECKS */

    /* Fields for maintaining list of tagged entries */
    H5SL_t *                    tag_list;
    hbool_t                     ignore_tags;

    /* Fields for tracking protected entries */
    uint32_t                    pl_len;
    size_t                      pl_size;
    H5C_cache_entry_t *	        pl_head_ptr;
    H5C_cache_entry_t *  	pl_tail_ptr;

    /* Fields for tracking pinned entries */
    uint32_t                    pel_len;
    size_t                      pel_size;
    H5C_cache_entry_t *	        pel_head_ptr;
    H5C_cache_entry_t *  	pel_tail_ptr;

    /* Fields for complete LRU list of entries */
    uint32_t                    LRU_list_len;
    size_t                      LRU_list_size;
    H5C_cache_entry_t *		LRU_head_ptr;
    H5C_cache_entry_t *		LRU_tail_ptr;

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS
    /* Fields for clean LRU list of entries */
    uint32_t                    cLRU_list_len;
    size_t                      cLRU_list_size;
    H5C_cache_entry_t *		cLRU_head_ptr;
    H5C_cache_entry_t *		cLRU_tail_ptr;

    /* Fields for dirty LRU list of entries */
    uint32_t                    dLRU_list_len;
    size_t                      dLRU_list_size;
    H5C_cache_entry_t *		dLRU_head_ptr;
    H5C_cache_entry_t *	        dLRU_tail_ptr;
#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

#ifdef H5_HAVE_PARALLEL
    /* Fields for collective metadata reads */
    uint32_t                    coll_list_len;
    size_t                      coll_list_size;
    H5C_cache_entry_t *		coll_head_ptr;
    H5C_cache_entry_t *		coll_tail_ptr;

    /* Fields for collective metadata writes */
    H5SL_t *                    coll_write_list;
#endif /* H5_HAVE_PARALLEL */

    /* Fields for automatic cache size adjustment */
    hbool_t			size_increase_possible;
    hbool_t			flash_size_increase_possible;
    size_t			flash_size_increase_threshold;
    hbool_t			size_decrease_possible;
    hbool_t			resize_enabled;
    hbool_t			cache_full;
    hbool_t			size_decreased;
    hbool_t			resize_in_progress;
    hbool_t			msic_in_progress;
    H5C_auto_size_ctl_t		resize_ctl;

    /* Fields for epoch markers used in automatic cache size adjustment */
    int32_t			epoch_markers_active;
    hbool_t			epoch_marker_active[H5C__MAX_EPOCH_MARKERS];
    int32_t			epoch_marker_ringbuf[H5C__MAX_EPOCH_MARKERS+1];
    int32_t			epoch_marker_ringbuf_first;
    int32_t			epoch_marker_ringbuf_last;
    int32_t			epoch_marker_ringbuf_size;
    H5C_cache_entry_t		epoch_markers[H5C__MAX_EPOCH_MARKERS];

    /* Fields for cache hit rate collection */
    int64_t			cache_hits;
    int64_t			cache_accesses;

    /* fields supporting generation of a cache image on file close */
    H5C_cache_image_ctl_t	image_ctl;
    hbool_t			serialization_in_progress;
    hbool_t			load_image;
    hbool_t                     image_loaded;
    hbool_t			delete_image;
    haddr_t 			image_addr;
    hsize_t			image_len;
    hsize_t			image_data_len;
    int64_t			entries_loaded_counter;
    int64_t			entries_inserted_counter;
    int64_t			entries_relocated_counter;
    int64_t			entry_fd_height_change_counter;
    uint32_t			num_entries_in_image;
    H5C_image_entry_t *		image_entries;
    void *                      image_buffer;

    /* Free Space Manager Related fields */
    hbool_t 			rdfsm_settled;
    hbool_t			mdfsm_settled;

#if H5C_COLLECT_CACHE_STATS
    /* stats fields */
    int64_t                     hits[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     misses[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     write_protects[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     read_protects[H5C__MAX_NUM_TYPE_IDS + 1];
    int32_t                     max_read_protects[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     insertions[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     pinned_insertions[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     clears[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     flushes[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     evictions[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     take_ownerships[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     moves[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     entry_flush_moves[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     cache_flush_moves[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     pins[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     unpins[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     dirty_pins[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     pinned_flushes[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     pinned_clears[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     size_increases[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     size_decreases[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     entry_flush_size_changes[H5C__MAX_NUM_TYPE_IDS + 1];
    int64_t                     cache_flush_size_changes[H5C__MAX_NUM_TYPE_IDS + 1];

    /* Fields for hash table operations */
    int64_t			total_ht_insertions;
    int64_t			total_ht_deletions;
    int64_t			successful_ht_searches;
    int64_t			total_successful_ht_search_depth;
    int64_t			failed_ht_searches;
    int64_t			total_failed_ht_search_depth;
    uint32_t                    max_index_len;
    size_t                      max_index_size;
    size_t                      max_clean_index_size;
    size_t                      max_dirty_index_size;

    /* Fields for in-order skip list */
    uint32_t                    max_slist_len;
    size_t                      max_slist_size;

    /* Fields for protected entry list */
    uint32_t                    max_pl_len;
    size_t                      max_pl_size;

    /* Fields for pinned entry list */
    uint32_t                    max_pel_len;
    size_t                      max_pel_size;

    /* Fields for tracking 'make space in cache' (msic) operations */
    int64_t                     calls_to_msic;
    int64_t                     total_entries_skipped_in_msic;
    int64_t                     total_dirty_pf_entries_skipped_in_msic;
    int64_t                     total_entries_scanned_in_msic;
    int32_t                     max_entries_skipped_in_msic;
    int32_t                     max_dirty_pf_entries_skipped_in_msic;
    int32_t                     max_entries_scanned_in_msic;
    int64_t                     entries_scanned_to_make_space;
 
    /* Fields for tracking skip list scan restarts */
    int64_t			slist_scan_restarts;
    int64_t			LRU_scan_restarts;
    int64_t			index_scan_restarts;

    /* Fields for tracking cache image operations */
    int32_t			images_created;
    int32_t			images_read;
    int32_t			images_loaded;
    hsize_t			last_image_size;

    /* Fields for tracking prefetched entries */
    int64_t			prefetches;
    int64_t			dirty_prefetches;
    int64_t			prefetch_hits;

#if H5C_COLLECT_CACHE_ENTRY_STATS
    int32_t                     max_accesses[H5C__MAX_NUM_TYPE_IDS + 1];
    int32_t                     min_accesses[H5C__MAX_NUM_TYPE_IDS + 1];
    int32_t                     max_clears[H5C__MAX_NUM_TYPE_IDS + 1];
    int32_t                     max_flushes[H5C__MAX_NUM_TYPE_IDS + 1];
    size_t                      max_size[H5C__MAX_NUM_TYPE_IDS + 1];
    int32_t                     max_pins[H5C__MAX_NUM_TYPE_IDS + 1];
#endif /* H5C_COLLECT_CACHE_ENTRY_STATS */
#endif /* H5C_COLLECT_CACHE_STATS */

    char			prefix[H5C__PREFIX_LEN];

#ifndef NDEBUG
    int64_t                     get_entry_ptr_from_addr_counter;
#endif /* NDEBUG */
};

/* Define typedef for tagged cache entry iteration callbacks */
typedef int (*H5C_tag_iter_cb_t)(H5C_cache_entry_t *entry, void *ctx);


/*****************************/
/* Package Private Variables */
/*****************************/


/******************************/
/* Package Private Prototypes */
/******************************/
H5_DLL herr_t H5C__prep_image_for_file_close(H5F_t *f, hbool_t *image_generated);
H5_DLL herr_t H5C__deserialize_prefetched_entry(H5F_t *f, H5C_t * cache_ptr,
    H5C_cache_entry_t** entry_ptr_ptr, const H5C_class_t * type, haddr_t addr,
    void * udata);

/* General routines */
H5_DLL herr_t H5C__flush_single_entry(H5F_t *f, H5C_cache_entry_t *entry_ptr,
    unsigned flags);
H5_DLL herr_t H5C__generate_cache_image(H5F_t *f, H5C_t *cache_ptr);
H5_DLL herr_t H5C__load_cache_image(H5F_t *f);
H5_DLL herr_t H5C__mark_flush_dep_serialized(H5C_cache_entry_t * entry_ptr);
H5_DLL herr_t H5C__mark_flush_dep_unserialized(H5C_cache_entry_t * entry_ptr);
H5_DLL herr_t H5C__make_space_in_cache(H5F_t * f, size_t  space_needed,
    hbool_t write_permitted);
H5_DLL herr_t H5C__flush_marked_entries(H5F_t * f);
H5_DLL herr_t H5C__generate_image(H5F_t *f, H5C_t *cache_ptr,
    H5C_cache_entry_t *entry_ptr);
H5_DLL herr_t H5C__serialize_cache(H5F_t *f);
H5_DLL herr_t H5C__iter_tagged_entries(H5C_t *cache, haddr_t tag, hbool_t match_global,
    H5C_tag_iter_cb_t cb, void *cb_ctx);

/* Routines for operating on entry tags */
H5_DLL herr_t H5C__tag_entry(H5C_t * cache_ptr, H5C_cache_entry_t * entry_ptr);
H5_DLL herr_t H5C__untag_entry(H5C_t *cache, H5C_cache_entry_t *entry);

/* Testing functions */
#ifdef H5C_TESTING
H5_DLL herr_t H5C__verify_cork_tag_test(hid_t fid, haddr_t tag, hbool_t status);
#endif /* H5C_TESTING */

#endif /* _H5Cpkg_H */

