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

/*-------------------------------------------------------------------------
 *
 * Created:             H5ACpublic.h
 *                      Jul 10 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Public include file for cache functions.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5ACpublic_H
#define _H5ACpublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Cpublic.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 *
 * structure H5AC_cache_config_t
 *
 * H5AC_cache_config_t is a public structure intended for use in public APIs.
 * At least in its initial incarnation, it is basically a copy of struct
 * H5C_auto_size_ctl_t, minus the report_fcn field, and plus the
 * dirty_bytes_threshold field.
 *
 * The report_fcn field is omitted, as including it would require us to
 * make H5C_t structure public.
 *
 * The dirty_bytes_threshold field does not appear in H5C_auto_size_ctl_t,
 * as synchronization between caches on different processes is handled at
 * the H5AC level, not at the level of H5C.  Note however that there is
 * considerable interaction between this value and the other fields in this
 * structure.
 *
 * Similarly, the open_trace_file, close_trace_file, and trace_file_name
 * fields do not appear in H5C_auto_size_ctl_t, as most trace file
 * issues are handled at the H5AC level.  The one exception is storage of
 * the pointer to the trace file, which is handled by H5C.
 *
 * The structure is in H5ACpublic.h as we may wish to allow different
 * configuration options for metadata and raw data caches.
 *
 * The fields of the structure are discussed individually below:
 *
 * version: Integer field containing the version number of this version
 *      of the H5AC_cache_config_t structure.  Any instance of
 *      H5AC_cache_config_t passed to the cache must have a known
 *      version number, or an error will be flagged.
 *
 * rpt_fcn_enabled: Boolean field used to enable and disable the default
 *    reporting function.  This function is invoked every time the
 *    automatic cache resize code is run, and reports on its activities.
 *
 *    This is a debugging function, and should normally be turned off.
 *
 * open_trace_file: Boolean field indicating whether the trace_file_name
 *     field should be used to open a trace file for the cache.
 *
 *      *** DEPRECATED *** Use H5Fstart/stop logging functions instead
 *
 *     The trace file is a debuging feature that allow the capture of
 *     top level metadata cache requests for purposes of debugging and/or
 *     optimization.  This field should normally be set to FALSE, as
 *     trace file collection imposes considerable overhead.
 *
 *     This field should only be set to TRUE when the trace_file_name
 *     contains the full path of the desired trace file, and either
 *     there is no open trace file on the cache, or the close_trace_file
 *     field is also TRUE.
 *
 * close_trace_file: Boolean field indicating whether the current trace
 *     file (if any) should be closed.
 *
 *      *** DEPRECATED *** Use H5Fstart/stop logging functions instead
 *
 *     See the above comments on the open_trace_file field.  This field
 *     should be set to FALSE unless there is an open trace file on the
 *     cache that you wish to close.
 *
 * trace_file_name: Full path of the trace file to be opened if the
 *     open_trace_file field is TRUE.
 *
 *      *** DEPRECATED *** Use H5Fstart/stop logging functions instead
 *
 *     In the parallel case, an ascii representation of the mpi rank of
 *     the process will be appended to the file name to yield a unique
 *     trace file name for each process.
 *
 *     The length of the path must not exceed H5AC__MAX_TRACE_FILE_NAME_LEN
 *     characters.
 *
 * evictions_enabled:  Boolean field used to either report the current
 *     evictions enabled status of the cache, or to set the cache's
 *    evictions enabled status.
 *
 *     In general, the metadata cache should always be allowed to
 *     evict entries.  However, in some cases it is advantageous to
 *     disable evictions briefly, and thereby postpone metadata
 *     writes.  However, this must be done with care, as the cache
 *     can grow quickly.  If you do this, re-enable evictions as
 *     soon as possible and monitor cache size.
 *
 *     At present, evictions can only be disabled if automatic
 *     cache resizing is also disabled (that is, ( incr_mode ==
 *    H5C_incr__off ) && ( decr_mode == H5C_decr__off )).  There
 *    is no logical reason why this should be so, but it simplifies
 *    implementation and testing, and I can't think of any reason
 *    why it would be desireable.  If you can think of one, I'll
 *    revisit the issue.
 *
 * set_initial_size: Boolean flag indicating whether the size of the
 *      initial size of the cache is to be set to the value given in
 *      the initial_size field.  If set_initial_size is FALSE, the
 *      initial_size field is ignored.
 *
 * initial_size: If enabled, this field contain the size the cache is
 *      to be set to upon receipt of this structure.  Needless to say,
 *      initial_size must lie in the closed interval [min_size, max_size].
 *
 * min_clean_fraction: double in the range 0 to 1 indicating the fraction
 *      of the cache that is to be kept clean.  This field is only used
 *      in parallel mode.  Typical values are 0.1 to 0.5.
 *
 * max_size: Maximum size to which the cache can be adjusted.  The
 *      supplied value must fall in the closed interval
 *      [MIN_MAX_CACHE_SIZE, MAX_MAX_CACHE_SIZE].  Also, max_size must
 *      be greater than or equal to min_size.
 *
 * min_size: Minimum size to which the cache can be adjusted.  The
 *      supplied value must fall in the closed interval
 *      [H5C__MIN_MAX_CACHE_SIZE, H5C__MAX_MAX_CACHE_SIZE].  Also, min_size
 *      must be less than or equal to max_size.
 *
 * epoch_length: Number of accesses on the cache over which to collect
 *      hit rate stats before running the automatic cache resize code,
 *      if it is enabled.
 *
 *      At the end of an epoch, we discard prior hit rate data and start
 *      collecting afresh.  The epoch_length must lie in the closed
 *      interval [H5C__MIN_AR_EPOCH_LENGTH, H5C__MAX_AR_EPOCH_LENGTH].
 *
 *
 * Cache size increase control fields:
 *
 * incr_mode: Instance of the H5C_cache_incr_mode enumerated type whose
 *      value indicates how we determine whether the cache size should be
 *      increased.  At present there are two possible values:
 *
 *      H5C_incr__off:  Don't attempt to increase the size of the cache
 *              automatically.
 *
 *              When this increment mode is selected, the remaining fields
 *              in the cache size increase section ar ignored.
 *
 *      H5C_incr__threshold: Attempt to increase the size of the cache
 *              whenever the average hit rate over the last epoch drops
 *              below the value supplied in the lower_hr_threshold
 *              field.
 *
 *              Note that this attempt will fail if the cache is already
 *              at its maximum size, or if the cache is not already using
 *              all available space.
 *
 *      Note that you must set decr_mode to H5C_incr__off if you
 *      disable metadata cache entry evictions.
 *
 * lower_hr_threshold: Lower hit rate threshold.  If the increment mode
 *      (incr_mode) is H5C_incr__threshold and the hit rate drops below the
 *      value supplied in this field in an epoch, increment the cache size by
 *      size_increment.  Note that cache size may not be incremented above
 *      max_size, and that the increment may be further restricted by the
 *      max_increment field if it is enabled.
 *
 *      When enabled, this field must contain a value in the range [0.0, 1.0].
 *      Depending on the incr_mode selected, it may also have to be less than
 *      upper_hr_threshold.
 *
 * increment:  Double containing the multiplier used to derive the new
 *      cache size from the old if a cache size increment is triggered.
 *      The increment must be greater than 1.0, and should not exceed 2.0.
 *
 *      The new cache size is obtained my multiplying the current max cache
 *      size by the increment, and then clamping to max_size and to stay
 *      within the max_increment as necessary.
 *
 * apply_max_increment:  Boolean flag indicating whether the max_increment
 *      field should be used to limit the maximum cache size increment.
 *
 * max_increment: If enabled by the apply_max_increment field described
 *      above, this field contains the maximum number of bytes by which the
 *      cache size can be increased in a single re-size.
 *
 * flash_incr_mode:  Instance of the H5C_cache_flash_incr_mode enumerated
 *      type whose value indicates whether and by which algorithm we should
 *      make flash increases in the size of the cache to accommodate insertion
 *      of large entries and large increases in the size of a single entry.
 *
 *      The addition of the flash increment mode was occasioned by performance
 *      problems that appear when a local heap is increased to a size in excess
 *      of the current cache size.  While the existing re-size code dealt with
 *      this eventually, performance was very bad for the remainder of the
 *      epoch.
 *
 *      At present, there are two possible values for the flash_incr_mode:
 *
 *      H5C_flash_incr__off:  Don't perform flash increases in the size of
 *              the cache.
 *
 *      H5C_flash_incr__add_space:  Let x be either the size of a newly
 *              newly inserted entry, or the number of bytes by which the
 *              size of an existing entry has been increased.
 *
 *              If
 *                      x > flash_threshold * current max cache size,
 *
 *              increase the current maximum cache size by x * flash_multiple
 *              less any free space in the cache, and star a new epoch.  For
 *              now at least, pay no attention to the maximum increment.
 *
 *      In both of the above cases, the flash increment pays no attention to
 *      the maximum increment (at least in this first incarnation), but DOES
 *      stay within max_size.
 *
 *      With a little thought, it should be obvious that the above flash
 *      cache size increase algorithm is not sufficient for all circumstances
 *      -- for example, suppose the user round robins through
 *      (1/flash_threshold) +1 groups, adding one data set to each on each
 *      pass.  Then all will increase in size at about the same time, requiring
 *      the max cache size to at least double to maintain acceptable
 *      performance, however the above flash increment algorithm will not be
 *      triggered.
 *
 *      Hopefully, the add space algorithms detailed above will be sufficient
 *      for the performance problems encountered to date.  However, we should
 *      expect to revisit the issue.
 *
 * flash_multiple: Double containing the multiple described above in the
 *      H5C_flash_incr__add_space section of the discussion of the
 *      flash_incr_mode section.  This field is ignored unless flash_incr_mode
 *      is H5C_flash_incr__add_space.
 *
 * flash_threshold: Double containing the factor by which current max cache
 *      size is multiplied to obtain the size threshold for the add_space flash
 *      increment algorithm.  The field is ignored unless flash_incr_mode is
 *      H5C_flash_incr__add_space.
 *
 *
 * Cache size decrease control fields:
 *
 * decr_mode: Instance of the H5C_cache_decr_mode enumerated type whose
 *      value indicates how we determine whether the cache size should be
 *      decreased.  At present there are four possibilities.
 *
 *      H5C_decr__off:  Don't attempt to decrease the size of the cache
 *              automatically.
 *
 *              When this increment mode is selected, the remaining fields
 *              in the cache size decrease section are ignored.
 *
 *      H5C_decr__threshold: Attempt to decrease the size of the cache
 *              whenever the average hit rate over the last epoch rises
 *              above the value supplied in the upper_hr_threshold
 *              field.
 *
 *      H5C_decr__age_out:  At the end of each epoch, search the cache for
 *              entries that have not been accessed for at least the number
 *              of epochs specified in the epochs_before_eviction field, and
 *              evict these entries.  Conceptually, the maximum cache size
 *              is then decreased to match the new actual cache size.  However,
 *              this reduction may be modified by the min_size, the
 *              max_decrement, and/or the empty_reserve.
 *
 *      H5C_decr__age_out_with_threshold:  Same as age_out, but we only
 *              attempt to reduce the cache size when the hit rate observed
 *              over the last epoch exceeds the value provided in the
 *              upper_hr_threshold field.
 *
 *      Note that you must set decr_mode to H5C_decr__off if you
 *      disable metadata cache entry evictions.
 *
 * upper_hr_threshold: Upper hit rate threshold.  The use of this field
 *      varies according to the current decr_mode:
 *
 *      H5C_decr__off or H5C_decr__age_out:  The value of this field is
 *              ignored.
 *
 *      H5C_decr__threshold:  If the hit rate exceeds this threshold in any
 *              epoch, attempt to decrement the cache size by size_decrement.
 *
 *              Note that cache size may not be decremented below min_size.
 *
 *              Note also that if the upper_threshold is 1.0, the cache size
 *              will never be reduced.
 *
 *      H5C_decr__age_out_with_threshold:  If the hit rate exceeds this
 *              threshold in any epoch, attempt to reduce the cache size
 *              by evicting entries that have not been accessed for more
 *              than the specified number of epochs.
 *
 * decrement: This field is only used when the decr_mode is
 *      H5C_decr__threshold.
 *
 *      The field is a double containing the multiplier used to derive the
 *      new cache size from the old if a cache size decrement is triggered.
 *      The decrement must be in the range 0.0 (in which case the cache will
 *      try to contract to its minimum size) to 1.0 (in which case the
 *      cache will never shrink).
 *
 * apply_max_decrement:  Boolean flag used to determine whether decrements
 *      in cache size are to be limited by the max_decrement field.
 *
 * max_decrement: Maximum number of bytes by which the cache size can be
 *      decreased in a single re-size.  Note that decrements may also be
 *      restricted by the min_size of the cache, and (in age out modes) by
 *      the empty_reserve field.
 *
 * epochs_before_eviction:  Integer field used in H5C_decr__age_out and
 *      H5C_decr__age_out_with_threshold decrement modes.
 *
 *      This field contains the number of epochs an entry must remain
 *      unaccessed before it is evicted in an attempt to reduce the
 *      cache size.  If applicable, this field must lie in the range
 *      [1, H5C__MAX_EPOCH_MARKERS].
 *
 * apply_empty_reserve:  Boolean field controlling whether the empty_reserve
 *      field is to be used in computing the new cache size when the
 *      decr_mode is H5C_decr__age_out or H5C_decr__age_out_with_threshold.
 *
 * empty_reserve:  To avoid a constant racheting down of cache size by small
 *      amounts in the H5C_decr__age_out and H5C_decr__age_out_with_threshold
 *      modes, this field allows one to require that any cache size
 *      reductions leave the specified fraction of unused space in the cache.
 *
 *      The value of this field must be in the range [0.0, 1.0].  I would
 *      expect typical values to be in the range of 0.01 to 0.1.
 *
 *
 * Parallel Configuration Fields:
 *
 * In PHDF5, all operations that modify metadata must be executed collectively.
 *
 * We used to think that this was enough to ensure consistency across the
 * metadata caches, but since we allow processes to read metadata individually,
 * the order of dirty entries in the LRU list can vary across processes,
 * which can result in inconsistencies between the caches.
 *
 * PHDF5 uses several strategies to prevent such inconsistencies in metadata,
 * all of which use the fact that the same stream of dirty metadata is seen
 * by all processes for purposes of synchronization.  This is done by
 * having each process count the number of bytes of dirty metadata generated,
 * and then running a "sync point" whenever this count exceeds a user
 * specified threshold (see dirty_bytes_threshold below).
 *
 * The current metadata write strategy is indicated by the
 * metadata_write_strategy field.  The possible values of this field, along
 * with the associated metadata write strategies are discussed below.
 *
 * dirty_bytes_threshold:  Threshold of dirty byte creation used to
 *     synchronize updates between caches. (See above for outline and
 *    motivation.)
 *
 *    This value MUST be consistent across all processes accessing the
 *    file.  This field is ignored unless HDF5 has been compiled for
 *    parallel.
 *
 * metadata_write_strategy: Integer field containing a code indicating the
 *    desired metadata write strategy.  The valid values of this field
 *    are enumerated and discussed below:
 *
 *
 *    H5AC_METADATA_WRITE_STRATEGY__PROCESS_0_ONLY:
 *
 *    When metadata_write_strategy is set to this value, only process
 *    zero is allowed to write dirty metadata to disk.  All other
 *    processes must retain dirty metadata until they are informed at
 *    a sync point that the dirty metadata in question has been written
 *    to disk.
 *
 *    When the sync point is reached (or when there is a user generated
 *    flush), process zero flushes sufficient entries to bring it into
 *    complience with its min clean size (or flushes all dirty entries in
 *    the case of a user generated flush), broad casts the list of
 *    entries just cleaned to all the other processes, and then exits
 *    the sync point.
 *
 *    Upon receipt of the broadcast, the other processes mark the indicated
 *    entries as clean, and leave the sync point as well.
 *
 *
 *    H5AC_METADATA_WRITE_STRATEGY__DISTRIBUTED:
 *
 *    In the distributed metadata write strategy, process zero still makes
 *    the decisions as to what entries should be flushed, but the actual
 *    flushes are distributed across the processes in the computation to
 *    the extent possible.
 *
 *    In this strategy, when a sync point is triggered (either by dirty
 *    metadata creation or manual flush), all processes enter a barrier.
 *
 *    On the other side of the barrier, process 0 constructs an ordered
 *    list of the entries to be flushed, and then broadcasts this list
 *    to the caches in all the processes.
 *
 *    All processes then scan the list of entries to be flushed, flushing
 *    some, and marking the rest as clean.  The algorithm for this purpose
 *    ensures that each entry in the list is flushed exactly once, and
 *    all are marked clean in each cache.
 *
 *    Note that in the case of a flush of the cache, no message passing
 *    is necessary, as all processes have the same list of dirty entries,
 *    and all of these entries must be flushed.  Thus in this case it is
 *    sufficient for each process to sort its list of dirty entries after
 *    leaving the initial barrier, and use this list as if it had been
 *    received from process zero.
 *
 *    To avoid possible messages from the past/future, all caches must
 *    wait until all caches are done before leaving the sync point.
 *
 ****************************************************************************/

#define H5AC__CURR_CACHE_CONFIG_VERSION     1
#define H5AC__MAX_TRACE_FILE_NAME_LEN        1024

#define H5AC_METADATA_WRITE_STRATEGY__PROCESS_0_ONLY    0
#define H5AC_METADATA_WRITE_STRATEGY__DISTRIBUTED       1

typedef struct H5AC_cache_config_t
{
    /* general configuration fields: */
    int                      version;

    hbool_t             rpt_fcn_enabled;

    hbool_t             open_trace_file;
    hbool_t                  close_trace_file;
    char                     trace_file_name[H5AC__MAX_TRACE_FILE_NAME_LEN + 1];

    hbool_t                  evictions_enabled;

    hbool_t                  set_initial_size;
    size_t                   initial_size;

    double                   min_clean_fraction;

    size_t                   max_size;
    size_t                   min_size;

    long int                 epoch_length;


    /* size increase control fields: */
    enum H5C_cache_incr_mode incr_mode;

    double                   lower_hr_threshold;

    double                   increment;

    hbool_t                  apply_max_increment;
    size_t                   max_increment;

    enum H5C_cache_flash_incr_mode      flash_incr_mode;
    double                              flash_multiple;
    double                              flash_threshold;


    /* size decrease control fields: */
    enum H5C_cache_decr_mode decr_mode;

    double                   upper_hr_threshold;

    double                   decrement;

    hbool_t                  apply_max_decrement;
    size_t                   max_decrement;

    int                      epochs_before_eviction;

    hbool_t                  apply_empty_reserve;
    double                   empty_reserve;


    /* parallel configuration fields: */
    size_t                   dirty_bytes_threshold;
    int                      metadata_write_strategy;

} H5AC_cache_config_t;


/****************************************************************************
 *
 * structure H5AC_cache_image_config_t
 *
 * H5AC_cache_image_ctl_t is a public structure intended for use in public
 * APIs.  At least in its initial incarnation, it is a copy of struct
 * H5C_cache_image_ctl_t.
 *
 * The fields of the structure are discussed individually below:
 *
 * version: Integer field containing the version number of this version
 *      of the H5C_image_ctl_t structure.  Any instance of
 *      H5C_image_ctl_t passed to the cache must have a known
 *      version number, or an error will be flagged.
 *
 * generate_image:  Boolean flag indicating whether a cache image should
 *      be created on file close.
 *
 * save_resize_status:    Boolean flag indicating whether the cache image
 *    should include the adaptive cache resize configuration and status.
 *    Note that this field is ignored at present.
 *
 * entry_ageout:    Integer field indicating the maximum number of
 *    times a prefetched entry can appear in subsequent cache images.
 *    This field exists to allow the user to avoid the buildup of
 *    infrequently used entries in long sequences of cache images.
 *
 *    The value of this field must lie in the range
 *    H5AC__CACHE_IMAGE__ENTRY_AGEOUT__NONE (-1) to
 *    H5AC__CACHE_IMAGE__ENTRY_AGEOUT__MAX (100).
 *
 *    H5AC__CACHE_IMAGE__ENTRY_AGEOUT__NONE means that no limit
 *    is imposed on number of times a prefeteched entry can appear
 *    in subsequent cache images.
 *
 *    A value of 0 prevents prefetched entries from being included
 *    in cache images.
 *
 *    Positive integers restrict prefetched entries to the specified
 *    number of appearances.
 *
 *    Note that the number of subsequent cache images that a prefetched
 *    entry has appeared in is tracked in an 8 bit field.  Thus, while
 *    H5AC__CACHE_IMAGE__ENTRY_AGEOUT__MAX can be increased from its
 *    current value, any value in excess of 255 will be the functional
 *    equivalent of H5AC__CACHE_IMAGE__ENTRY_AGEOUT__NONE.
 *
 ****************************************************************************/

#define H5AC__CURR_CACHE_IMAGE_CONFIG_VERSION     1

#define H5AC__CACHE_IMAGE__ENTRY_AGEOUT__NONE    -1
#define H5AC__CACHE_IMAGE__ENTRY_AGEOUT__MAX    100

typedef struct H5AC_cache_image_config_t {
    int                                 version;
    hbool_t                             generate_image;
    hbool_t                             save_resize_status;
    int                                 entry_ageout;
} H5AC_cache_image_config_t;

#ifdef __cplusplus
}
#endif
#endif

