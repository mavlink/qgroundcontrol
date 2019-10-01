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
 * Programmer: John Mainzer -- 4/19/06
 *
 * Purpose:     This file contains declarations which are normally visible
 *              only within the H5AC package (just H5AC.c at present).
 *
 *		Source files outside the H5AC package should include
 *		H5ACprivate.h instead.
 *
 *		The one exception to this rule is testpar/t_cache.c.  The
 *		test code is easier to write if it can look at H5AC_aux_t.
 *		Indeed, this is the main reason why this file was created.
 *
 */

#if !(defined H5AC_FRIEND || defined H5AC_MODULE)
#error "Do not include this file outside the H5AC package!"
#endif

#ifndef _H5ACpkg_H
#define _H5ACpkg_H

/* Get package's private header */
#include "H5ACprivate.h"	/* Metadata cache			*/


/* Get needed headers */
#include "H5Cprivate.h"         /* Cache                                */
#include "H5FLprivate.h"        /* Free Lists                           */

/*****************************/
/* Package Private Variables */
/*****************************/

/* Declare extern the free list to manage the H5AC_aux_t struct */
H5FL_EXTERN(H5AC_aux_t);


/**************************/
/* Package Private Macros */
/**************************/

#define H5AC_DEBUG_DIRTY_BYTES_CREATION	0

#ifdef H5_HAVE_PARALLEL

/* the following #defined are used to specify the operation required
 * at a sync point.
 */

#define H5AC_SYNC_POINT_OP__FLUSH_TO_MIN_CLEAN		0
#define H5AC_SYNC_POINT_OP__FLUSH_CACHE			1

#endif /* H5_HAVE_PARALLEL */

/*-------------------------------------------------------------------------
 *  It is a bit difficult to set ranges of allowable values on the
 *  dirty_bytes_threshold field of H5AC_aux_t.  The following are
 *  probably broader than they should be.
 *-------------------------------------------------------------------------
 */

#define H5AC__MIN_DIRTY_BYTES_THRESHOLD		(size_t) \
						(H5C__MIN_MAX_CACHE_SIZE / 2)
#define H5AC__DEFAULT_DIRTY_BYTES_THRESHOLD	(256 * 1024)
#define H5AC__MAX_DIRTY_BYTES_THRESHOLD   	(size_t) \
						(H5C__MAX_MAX_CACHE_SIZE / 4)


/****************************************************************************
 *
 * structure H5AC_aux_t
 *
 * While H5AC has become a wrapper for the cache implemented in H5C.c, there
 * are some features of the metadata cache that are specific to it, and which
 * therefore do not belong in the more generic H5C cache code.
 *
 * In particular, there is the matter of synchronizing writes from the
 * metadata cache to disk in the PHDF5 case.
 *
 * Prior to this update, the presumption was that all metadata caches would
 * write the same data at the same time since all operations modifying
 * metadata must be performed collectively.  Given this assumption, it was
 * safe to allow only the writes from process 0 to actually make it to disk,
 * while metadata writes from all other processes were discarded.
 *
 * Unfortunately, this presumption is in error as operations that read
 * metadata need not be collective, but can change the location of dirty
 * entries in the metadata cache LRU lists.  This can result in the same
 * metadata write operation triggering writes from the metadata caches on
 * some processes, but not all (causing a hang), or in different sets of
 * entries being written from different caches (potentially resulting in
 * metadata corruption in the file).
 *
 * To deal with this issue, I decided to apply a paradigm shift to the way
 * metadata is written to disk.
 *
 * With this set of changes, only the metadata cache on process 0 is able
 * to write metadata to disk, although metadata caches on all other
 * processes can read metadata from disk as before.
 *
 * To keep all the other caches from getting plugged up with dirty metadata,
 * process 0 periodically broadcasts a list of entries that it has flushed
 * since that last notice, and which are currently clean.  The other caches
 * mark these entries as clean as well, which allows them to evict the
 * entries as needed.
 *
 * One obvious problem in this approach is synchronizing the broadcasts
 * and receptions, as different caches may see different amounts of
 * activity.
 *
 * The current solution is for the caches to track the number of bytes
 * of newly generated dirty metadata, and to broadcast and receive
 * whenever this value exceeds some user specified threshold.
 *
 * Maintaining this count is easy for all processes not on process 0 --
 * all that is necessary is to add the size of the entry to the total
 * whenever there is an insertion, a move of a previously clean entry,
 * or whever a previously clean entry is marked dirty in an unprotect.
 *
 * On process 0, we have to be careful not to count dirty bytes twice.
 * If an entry is marked dirty, flushed, and marked dirty again, all
 * within a single reporting period, it only th first marking should
 * be added to the dirty bytes generated tally, as that is all that
 * the other processes will see.
 *
 * At present, this structure exists to maintain the fields needed to
 * implement the above scheme, and thus is only used in the parallel
 * case.  However, other uses may arise in the future.
 *
 * Instance of this structure are associated with metadata caches via
 * the aux_ptr field of H5C_t (see H5Cpkg.h).  The H5AC code is
 * responsible for allocating, maintaining, and discarding instances
 * of H5AC_aux_t.
 *
 * The remainder of this header comments documents the individual fields
 * of the structure.
 *
 *                                              JRM - 6/27/05
 *
 * Update: When the above was written, I planned to allow the process
 *	0 metadata cache to write dirty metadata between sync points.
 *	However, testing indicated that this allowed occasional 
 *	messages from the future to reach the caches on other processes.
 *
 *	To resolve this, the code was altered to require that all metadata
 *	writes take place during sync points -- which solved the problem.
 *	Initially all writes were performed by the process 0 cache.  This 
 *	approach was later replaced with a distributed write approach
 *	in which each process writes a subset of the metadata to be 
 *	written.  
 *
 *	After thinking on the matter for a while, I arrived at the 
 *	conclusion that the process 0 cache could be allowed to write 
 *	dirty metadata between sync points if it restricted itself to 
 *	entries that had been dirty at the time of the previous sync point.  
 *	
 *	To date, there has been no attempt to implement this optimization.
 *	However, should it be attempted, much of the supporting code 
 *	should still be around.
 *
 *						JRM -- 1/6/15
 *
 * magic:       Unsigned 32 bit integer always set to
 *		H5AC__H5AC_AUX_T_MAGIC.  This field is used to validate
 *		pointers to instances of H5AC_aux_t.
 *
 * mpi_comm:	MPI communicator associated with the file for which the
 *		cache has been created.
 *
 * mpi_rank:	MPI rank of this process within mpi_comm.
 *
 * mpi_size:	Number of processes in mpi_comm.
 *
 * write_permitted:  Boolean flag used to control whether the cache
 *		is permitted to write to file.
 *
 * dirty_bytes_threshold: Integer field containing the dirty bytes
 *		generation threshold.  Whenever dirty byte creation
 *		exceeds this value, the metadata cache on process 0
 *		broadcasts a list of the entries it has flushed since
 *		the last broadcast (or since the beginning of execution)
 *		and which are currently clean (if they are still in the
 *		cache)
 *
 *		Similarly, metadata caches on processes other than process
 *		0 will attempt to receive a list of clean entries whenever
 *		the threshold is exceeded.
 *
 * dirty_bytes:  Integer field containing the number of bytes of dirty
 *		metadata generated since the beginning of the computation,
 *		or (more typically) since the last clean entries list
 *		broadcast.  This field is reset to zero after each such
 *		broadcast.
 *
 * metadata_write_strategy: Integer code indicating how we will be 
 *		writing the metadata.  In the first incarnation of 
 *		this code, all writes were done from process 0.  This
 *		field exists to facilitate experiments with other 
 *		strategies.
 *
 *		At present, this field must be set to either
 *		H5AC_METADATA_WRITE_STRATEGY__PROCESS_0_ONLY or 
 *		H5AC_METADATA_WRITE_STRATEGY__DISTRIBUTED.
 *
 * dirty_bytes_propagations: This field only exists when the
 *		H5AC_DEBUG_DIRTY_BYTES_CREATION #define is TRUE.
 *
 *		It is used to track the number of times the cleaned list
 *		has been propagated from process 0 to the other
 *		processes.
 *
 * unprotect_dirty_bytes:  This field only exists when the
 *              H5AC_DEBUG_DIRTY_BYTES_CREATION #define is TRUE.
 *
 *		It is used to track the number of dirty bytes created
 *		via unprotect operations since the last time the cleaned
 *		list was propagated.
 *
 * unprotect_dirty_bytes_updates: This field only exists when the
 *              H5AC_DEBUG_DIRTY_BYTES_CREATION #define is TRUE.
 *
 *		It is used to track the number of times dirty bytes have
 *		been created via unprotect operations since the last time
 *		the cleaned list was propagated.
 *
 * insert_dirty_bytes:  This field only exists when the
 *              H5AC_DEBUG_DIRTY_BYTES_CREATION #define is TRUE.
 *
 *		It is used to track the number of dirty bytes created
 *		via insert operations since the last time the cleaned
 *		list was propagated.
 *
 * insert_dirty_bytes_updates:  This field only exists when the
 *              H5AC_DEBUG_DIRTY_BYTES_CREATION #define is TRUE.
 *
 *		It is used to track the number of times dirty bytes have
 *		been created via insert operations since the last time
 *		the cleaned list was propagated.
 *
 * move_dirty_bytes:  This field only exists when the
 *              H5AC_DEBUG_DIRTY_BYTES_CREATION #define is TRUE.
 *
 *		It is used to track the number of dirty bytes created
 *		via move operations since the last time the cleaned
 *		list was propagated.
 *
 * move_dirty_bytes_updates:  This field only exists when the
 *              H5AC_DEBUG_DIRTY_BYTES_CREATION #define is TRUE.
 *
 *		It is used to track the number of times dirty bytes have
 *		been created via move operations since the last time
 *		the cleaned list was propagated.
 *
 * Things have changed a bit since the following four fields were defined.
 * If metadata_write_strategy is H5AC_METADATA_WRITE_STRATEGY__PROCESS_0_ONLY,
 * all comments hold as before -- with the caviate that pending further 
 * coding, the process 0 metadata cache is forbidden to flush entries outside
 * of a sync point.
 *
 * However, for different metadata write strategies, these fields are used
 * only to maintain the correct dirty byte count on process zero -- and in
 * most if not all cases, this is redundant, as process zero will be barred
 * from flushing entries outside of a sync point.
 *
 *						JRM -- 3/16/10
 *
 * d_slist_ptr:  Pointer to an instance of H5SL_t used to maintain a list
 *		of entries that have been dirtied since the last time they
 *		were listed in a clean entries broadcast.  This list is
 *		only maintained by the metadata cache on process 0 -- it
 *		it used to maintain a view of the dirty entries as seen
 *		by the other caches, so as to keep the dirty bytes count
 *		in synchronization with them.
 *
 *		Thus on process 0, the dirty_bytes count is incremented
 *		only if either
 *
 *		1) an entry is inserted in the metadata cache, or
 *
 *		2) a previously clean entry is moved, and it does not
 *		   already appear in the dirty entry list, or
 *
 *		3) a previously clean entry is unprotected with the
 *		   dirtied flag set and the entry does not already appear
 *		   in the dirty entry list.
 *
 *		Entries are added to the dirty entry list whever they cause
 *		the dirty bytes count to be increased.  They are removed
 *		when they appear in a clean entries broadcast.  Note that
 *		moves must be reflected in the dirty entry list.
 *
 *		To reitterate, this field is only used on process 0 -- it
 *		should be NULL on all other processes.
 *
 * c_slist_ptr: Pointer to an instance of H5SL_t used to maintain a list
 *		of entries that were dirty, have been flushed
 *		to disk since the last clean entries broadcast, and are
 *		still clean.  Since only process 0 can write to disk, this
 *		list only exists on process 0.
 *
 *		In essence, this slist is used to assemble the contents of
 *		the next clean entries broadcast.  The list emptied after
 *		each broadcast.
 *
 * The following two fields are used only when metadata_write_strategy
 * is H5AC_METADATA_WRITE_STRATEGY__DISTRIBUTED.
 *
 * candidate_slist_ptr: Pointer to an instance of H5SL_t used by process 0
 *		to construct a list of entries to be flushed at this sync
 *		point.  This list is then broadcast to the other processes,
 *		which then either flush or mark clean all entries on it.
 *
 * write_done:  In the parallel test bed, it is necessary to ensure that
 *              all writes to the server process from cache 0 complete
 *              before it enters the barrier call with the other caches.
 *
 *              The write_done callback allows t_cache to do this without
 *              requiring an ACK on each write.  Since these ACKs greatly
 *              increase the run time on some platforms, this is a
 *              significant optimization.
 *
 *              This field must be set to NULL when the callback is not
 *              needed.
 *
 *		Note: This field has been extended for use by all processes
 *		      with the addition of support for the distributed 
 *		      metadata write strategy.        
 *                                                     JRM -- 5/9/10
 *
 * sync_point_done:  In the parallel test bed, it is necessary to verify
 *		that the expected writes, and only the expected writes,
 *		have taken place at the end of each sync point.
 *
 *		The sync_point_done callback allows t_cache to perform 
 *		this verification.  The field is set to NULL when the 
 *		callback is not needed.
 *
 * The following field supports the metadata cache image feature.
 *
 * p0_image_len: unsiged integer containing the length of the metadata cache
 *		image constructed by MPI process 0.  This field should be 0
 *		if the value is unknown, or if cache image is not enabled.
 *
 ****************************************************************************/

#ifdef H5_HAVE_PARALLEL

#define H5AC__H5AC_AUX_T_MAGIC        (unsigned)0x00D0A01

typedef struct H5AC_aux_t
{
    uint32_t	magic;

    MPI_Comm	mpi_comm;

    int		mpi_rank;

    int		mpi_size;

    hbool_t	write_permitted;

    size_t	dirty_bytes_threshold;

    size_t	dirty_bytes;

    int32_t	metadata_write_strategy;

#if H5AC_DEBUG_DIRTY_BYTES_CREATION

    unsigned	dirty_bytes_propagations;

    size_t      unprotect_dirty_bytes;
    unsigned    unprotect_dirty_bytes_updates;

    size_t      insert_dirty_bytes;
    unsigned    insert_dirty_bytes_updates;

    size_t      move_dirty_bytes;
    unsigned    move_dirty_bytes_updates;

#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */

    H5SL_t *	d_slist_ptr;

    H5SL_t *	c_slist_ptr;

    H5SL_t *	candidate_slist_ptr;

    void	(* write_done)(void);

    void	(* sync_point_done)(unsigned num_writes, 
                                    haddr_t * written_entries_tbl);

    unsigned    p0_image_len;

} H5AC_aux_t; /* struct H5AC_aux_t */
#endif /* H5_HAVE_PARALLEL */


/******************************/
/* Package Private Prototypes */
/******************************/

#ifdef H5_HAVE_PARALLEL
/* Parallel I/O routines */
H5_DLL herr_t H5AC__log_deleted_entry(const H5AC_info_t *entry_ptr);
H5_DLL herr_t H5AC__log_dirtied_entry(const H5AC_info_t *entry_ptr);
H5_DLL herr_t H5AC__log_cleaned_entry(const H5AC_info_t *entry_ptr);
H5_DLL herr_t H5AC__log_flushed_entry(H5C_t *cache_ptr, haddr_t addr,
    hbool_t was_dirty, unsigned flags);
H5_DLL herr_t H5AC__log_inserted_entry(const H5AC_info_t *entry_ptr);
H5_DLL herr_t H5AC__log_moved_entry(const H5F_t *f, haddr_t old_addr,
    haddr_t new_addr);
H5_DLL herr_t H5AC__flush_entries(H5F_t *f);
H5_DLL herr_t H5AC__run_sync_point(H5F_t *f, int sync_point_op);
H5_DLL herr_t H5AC__set_sync_point_done_callback(H5C_t *cache_ptr,
    void (*sync_point_done)(unsigned num_writes, haddr_t *written_entries_tbl));
H5_DLL herr_t H5AC__set_write_done_callback(H5C_t * cache_ptr,
    void (* write_done)(void));
#endif /* H5_HAVE_PARALLEL */

/* Trace file routines */
H5_DLL herr_t H5AC__close_trace_file(H5AC_t *cache_ptr);
H5_DLL herr_t H5AC__open_trace_file(H5AC_t *cache_ptr, const char *trace_file_name);

/* Cache logging routines */
H5_DLL herr_t H5AC__write_create_cache_log_msg(H5AC_t *cache);
H5_DLL herr_t H5AC__write_destroy_cache_log_msg(H5AC_t *cache);
H5_DLL herr_t H5AC__write_evict_cache_log_msg(const H5AC_t *cache,
                                        herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_expunge_entry_log_msg(const H5AC_t *cache,
                                                haddr_t address,
                                                int type_id,
                                                herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_flush_cache_log_msg(const H5AC_t *cache,
                                              herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_insert_entry_log_msg(const H5AC_t *cache,
                                               haddr_t address,
                                               int type_id,
                                               unsigned flags,
                                               size_t size,
                                               herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_mark_dirty_entry_log_msg(const H5AC_t *cache,
                                                   const H5AC_info_t *entry,
                                                   herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_mark_clean_entry_log_msg(const H5AC_t *cache,
    const H5AC_info_t *entry, herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_mark_unserialized_entry_log_msg(const H5AC_t *cache,
        const H5AC_info_t *entry, herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_mark_serialized_entry_log_msg(const H5AC_t *cache,
    const H5AC_info_t *entry, herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_move_entry_log_msg(const H5AC_t *cache,
                                             haddr_t old_addr,
                                             haddr_t new_addr,
                                             int type_id,
                                             herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_pin_entry_log_msg(const H5AC_t *cache,
                                            const H5AC_info_t *entry,
                                            herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_create_fd_log_msg(const H5AC_t *cache,
                                            const H5AC_info_t *parent,
                                            const H5AC_info_t *child,
                                            herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_protect_entry_log_msg(const H5AC_t *cache,
                                                const H5AC_info_t *entry,
                                                unsigned flags,
                                                herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_resize_entry_log_msg(const H5AC_t *cache,
                                               const H5AC_info_t *entry,
                                               size_t new_size,
                                               herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_unpin_entry_log_msg(const H5AC_t *cache,
                                              const H5AC_info_t *entry,
                                              herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_destroy_fd_log_msg(const H5AC_t *cache,
                                             const H5AC_info_t *parent,
                                             const H5AC_info_t *child,
                                             herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_unprotect_entry_log_msg(const H5AC_t *cache,
                                                  const H5AC_info_t *entry,
                                                  int type_id,
                                                  unsigned flags,
                                                  herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_set_cache_config_log_msg(const H5AC_t *cache,
                                                   const H5AC_cache_config_t *config,
                                                   herr_t fxn_ret_value);
H5_DLL herr_t H5AC__write_remove_entry_log_msg(const H5AC_t *cache,
                                              const H5AC_info_t *entry,
                                              herr_t fxn_ret_value);

#endif /* _H5ACpkg_H */

