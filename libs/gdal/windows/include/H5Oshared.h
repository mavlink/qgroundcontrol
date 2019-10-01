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
 * Programmer:	Quincey Koziol <koziol@hdfgroup.org>
 *		Friday, January 19, 2007
 *
 * Purpose:	This file contains inline definitions for "generic" routines
 *		supporting a "shared message interface" (ala Java) for object
 *		header messages that can be shared.  This interface is
 *              dependent on a bunch of macros being defined which define
 *              the name of the interface and "real" methods which need to
 *              be implemented for each message class that supports the
 *              shared message interface.
 */

#ifndef H5Oshared_H
#define H5Oshared_H


/*-------------------------------------------------------------------------
 * Function:    H5O_SHARED_DECODE
 *
 * Purpose:     Decode an object header message that may be shared.
 *
 * Note:	The actual name of this routine can be different in each source
 *		file that this header file is included in, and must be defined
 *		prior to including this header file.
 *
 * Return:      Success:        Pointer to the new message in native form
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Friday, January 19, 2007
 *
 *-------------------------------------------------------------------------
 */
static H5_INLINE void *
H5O_SHARED_DECODE(H5F_t *f, H5O_t *open_oh, unsigned mesg_flags,
    unsigned *ioflags, size_t p_size, const uint8_t *p)
{
    void *ret_value = NULL;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

#ifndef H5O_SHARED_TYPE
#error "Need to define H5O_SHARED_TYPE macro!"
#endif /* H5O_SHARED_TYPE */
#ifndef H5O_SHARED_DECODE
#error "Need to define H5O_SHARED_DECODE macro!"
#endif /* H5O_SHARED_DECODE */
#ifndef H5O_SHARED_DECODE_REAL
#error "Need to define H5O_SHARED_DECODE_REAL macro!"
#endif /* H5O_SHARED_DECODE_REAL */

    /* Check for shared message */
    if(mesg_flags & H5O_MSG_FLAG_SHARED) {
        /* Retrieve native message info indirectly through shared message */
        if(NULL == (ret_value = H5O_shared_decode(f, open_oh, ioflags, p, H5O_SHARED_TYPE)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDECODE, NULL, "unable to decode shared message")

        /* We currently do not support automatically fixing shared messages */
#ifdef H5_STRICT_FORMAT_CHECKS
        if(*ioflags & H5O_DECODEIO_DIRTY)
            HGOTO_ERROR(H5E_OHDR, H5E_UNSUPPORTED, NULL, "unable to mark shared message dirty")
#else /* H5_STRICT_FORMAT_CHECKS */
        *ioflags &= ~H5O_DECODEIO_DIRTY;
#endif /* H5_STRICT_FORMAT_CHECKS */
    } /* end if */
    else {
        /* Decode native message directly */
        if(NULL == (ret_value = H5O_SHARED_DECODE_REAL(f, open_oh, mesg_flags, ioflags, p_size, p)))
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDECODE, NULL, "unable to decode native message")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_SHARED_DECODE() */


/*-------------------------------------------------------------------------
 * Function:    H5O_SHARED_ENCODE
 *
 * Purpose:     Encode an object header message that may be shared.
 *
 * Note:	The actual name of this routine can be different in each source
 *		file that this header file is included in, and must be defined
 *		prior to including this header file.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, January 19, 2007
 *
 *-------------------------------------------------------------------------
 */
static H5_INLINE herr_t
H5O_SHARED_ENCODE(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg)
{
    const H5O_shared_t *sh_mesg = (const H5O_shared_t *)_mesg;     /* Pointer to shared message portion of actual message */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

#ifndef H5O_SHARED_TYPE
#error "Need to define H5O_SHARED_TYPE macro!"
#endif /* H5O_SHARED_TYPE */
#ifndef H5O_SHARED_ENCODE
#error "Need to define H5O_SHARED_ENCODE macro!"
#endif /* H5O_SHARED_ENCODE */
#ifndef H5O_SHARED_ENCODE_REAL
#error "Need to define H5O_SHARED_ENCODE_REAL macro!"
#endif /* H5O_SHARED_ENCODE_REAL */

    /* Sanity check */
    HDassert(sh_mesg->type == H5O_SHARE_TYPE_UNSHARED || sh_mesg->msg_type_id == H5O_SHARED_TYPE->id);

    /* Check for message stored elsewhere */
    if(H5O_IS_STORED_SHARED(sh_mesg->type) && !disable_shared) {
        /* Encode shared message into buffer */
        if(H5O_shared_encode(f, p, sh_mesg) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "unable to encode shared message")
    } /* end if */
    else {
        /* Encode native message directly */
        if(H5O_SHARED_ENCODE_REAL(f, p, _mesg) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "unable to encode native message")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_SHARED_ENCODE() */


/*-------------------------------------------------------------------------
 * Function:    H5O_SHARED_SIZE
 *
 * Purpose:	Returns the length of an encoded message.
 *
 * Note:	The actual name of this routine can be different in each source
 *		file that this header file is included in, and must be defined
 *		prior to including this header file.
 *
 * Return:	Success:	Length
 *		Failure:	0
 *
 * Programmer:  Quincey Koziol
 *              Friday, January 19, 2007
 *
 *-------------------------------------------------------------------------
 */
static H5_INLINE size_t
H5O_SHARED_SIZE(const H5F_t *f, hbool_t disable_shared, const void *_mesg)
{
    const H5O_shared_t *sh_mesg = (const H5O_shared_t *)_mesg;     /* Pointer to shared message portion of actual message */
    size_t ret_value = 0;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

#ifndef H5O_SHARED_TYPE
#error "Need to define H5O_SHARED_TYPE macro!"
#endif /* H5O_SHARED_TYPE */
#ifndef H5O_SHARED_SIZE
#error "Need to define H5O_SHARED_SIZE macro!"
#endif /* H5O_SHARED_SIZE */
#ifndef H5O_SHARED_SIZE_REAL
#error "Need to define H5O_SHARED_SIZE_REAL macro!"
#endif /* H5O_SHARED_SIZE_REAL */

    /* Check for message stored elsewhere */
    if(H5O_IS_STORED_SHARED(sh_mesg->type) && !disable_shared) {
        /* Retrieve encoded size of shared message */
        if(0 == (ret_value = H5O_shared_size(f, sh_mesg)))
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, 0, "unable to retrieve encoded size of shared message")
    } /* end if */
    else {
        /* Retrieve size of native message directly */
        if(0 == (ret_value = H5O_SHARED_SIZE_REAL(f, _mesg)))
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, 0, "unable to retrieve encoded size of native message")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_SHARED_SIZE() */


/*-------------------------------------------------------------------------
 * Function:    H5O_SHARED_DELETE
 *
 * Purpose:     Decrement reference count on any objects referenced by
 *              message
 *
 * Note:	The actual name of this routine can be different in each source
 *		file that this header file is included in, and must be defined
 *		prior to including this header file.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, January 19, 2007
 *
 *-------------------------------------------------------------------------
 */
static H5_INLINE herr_t
H5O_SHARED_DELETE(H5F_t *f, H5O_t *open_oh, void *_mesg)
{
    H5O_shared_t *sh_mesg = (H5O_shared_t *)_mesg;     /* Pointer to shared message portion of actual message */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

#ifndef H5O_SHARED_TYPE
#error "Need to define H5O_SHARED_TYPE macro!"
#endif /* H5O_SHARED_TYPE */
#ifndef H5O_SHARED_DELETE
#error "Need to define H5O_SHARED_DELETE macro!"
#endif /* H5O_SHARED_DELETE */

    /* Check for message tracked elsewhere */
    if(H5O_IS_TRACKED_SHARED(sh_mesg->type)) {
        /* Decrement the reference count on the shared message/object */
        if(H5O__shared_delete(f, open_oh, H5O_SHARED_TYPE, sh_mesg) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "unable to decrement ref count for shared message")
    } /* end if */
#ifdef H5O_SHARED_DELETE_REAL
    else {
        /* Decrement the reference count on the native message directly */
        if(H5O_SHARED_DELETE_REAL(f, open_oh, _mesg) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "unable to decrement ref count for native message")
    } /* end else */
#endif /* H5O_SHARED_DELETE_REAL */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_SHARED_DELETE() */


/*-------------------------------------------------------------------------
 * Function:    H5O_SHARED_LINK
 *
 * Purpose:     Increment reference count on any objects referenced by
 *              message
 *
 * Note:	The actual name of this routine can be different in each source
 *		file that this header file is included in, and must be defined
 *		prior to including this header file.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, January 19, 2007
 *
 *-------------------------------------------------------------------------
 */
static H5_INLINE herr_t
H5O_SHARED_LINK(H5F_t *f, H5O_t *open_oh, void *_mesg)
{
    H5O_shared_t *sh_mesg = (H5O_shared_t *)_mesg;     /* Pointer to shared message portion of actual message */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

#ifndef H5O_SHARED_TYPE
#error "Need to define H5O_SHARED_TYPE macro!"
#endif /* H5O_SHARED_TYPE */
#ifndef H5O_SHARED_LINK
#error "Need to define H5O_SHARED_LINK macro!"
#endif /* H5O_SHARED_LINK */

    /* Check for message tracked elsewhere */
    if(H5O_IS_TRACKED_SHARED(sh_mesg->type)) {
        /* Increment the reference count on the shared message/object */
        if(H5O__shared_link(f, open_oh, H5O_SHARED_TYPE, sh_mesg) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTINC, FAIL, "unable to increment ref count for shared message")
    } /* end if */
#ifdef H5O_SHARED_LINK_REAL
    else {
        /* Increment the reference count on the native message directly */
        if(H5O_SHARED_LINK_REAL(f, open_oh, _mesg) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTINC, FAIL, "unable to increment ref count for native message")
    } /* end else */
#endif /* H5O_SHARED_LINK_REAL */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_SHARED_LINK() */


/*-------------------------------------------------------------------------
 * Function:    H5O_SHARED_COPY_FILE
 *
 * Purpose:     Copies a message from _SRC to _DEST in file
 *
 * Note:	The actual name of this routine can be different in each source
 *		file that this header file is included in, and must be defined
 *		prior to including this header file.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, January 19, 2007
 *
 *-------------------------------------------------------------------------
 */
static H5_INLINE void *
H5O_SHARED_COPY_FILE(H5F_t *file_src, void *_native_src, H5F_t *file_dst,
    hbool_t *recompute_size, unsigned *mesg_flags, H5O_copy_t *cpy_info,
    void *udata)
{
    void *dst_mesg = NULL;      /* Destination message */
    void *ret_value = NULL;     /* Return value */

    FUNC_ENTER_STATIC

#ifndef H5O_SHARED_TYPE
#error "Need to define H5O_SHARED_TYPE macro!"
#endif /* H5O_SHARED_TYPE */
#ifndef H5O_SHARED_COPY_FILE
#error "Need to define H5O_SHARED_COPY_FILE macro!"
#endif /* H5O_SHARED_COPY_FILE */

#ifdef H5O_SHARED_COPY_FILE_REAL
    /* Call native message's copy file callback to copy the message */
    if(NULL == (dst_mesg = H5O_SHARED_COPY_FILE_REAL(file_src, H5O_SHARED_TYPE, _native_src, file_dst, recompute_size, cpy_info, udata)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy native message to another file")
#else /* H5O_SHARED_COPY_FILE_REAL */
    /* No copy file callback defined, just copy the message itself */
    if(NULL == (dst_mesg = (H5O_SHARED_TYPE->copy)(_native_src, NULL)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy native message")
#endif /* H5O_SHARED_COPY_FILE_REAL */

    /* Reset shared message info for new message */
    HDmemset(dst_mesg, 0, sizeof(H5O_shared_t));

    /* Handle sharing destination message */
    if(H5O__shared_copy_file(file_src, file_dst, H5O_SHARED_TYPE, _native_src,
            dst_mesg, recompute_size, mesg_flags, cpy_info, udata) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, NULL, "unable to determine if message should be shared")

    /* Set return value */
    ret_value = dst_mesg;

done:
    if(!ret_value)
        if(dst_mesg)
            H5O_msg_free(H5O_SHARED_TYPE->id, dst_mesg);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_SHARED_COPY_FILE() */


/*-------------------------------------------------------------------------
 * Function:    H5O_SHARED_POST_COPY_FILE
 *
 * Purpose:     Copies a message from _SRC to _DEST in file
 *
 * Note:        The actual name of this routine can be different in each source
 *              file that this header file is included in, and must be defined
 *              prior to including this header file.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Peter Cao
 *              May 25, 2007
 *
 *-------------------------------------------------------------------------
 */
static H5_INLINE herr_t
H5O_SHARED_POST_COPY_FILE(const H5O_loc_t *oloc_src, const void *mesg_src,
    H5O_loc_t *oloc_dst, void *mesg_dst, unsigned *mesg_flags,
    H5O_copy_t *cpy_info)
{
    const H5O_shared_t  *shared_src = (const H5O_shared_t *)mesg_src; /* Alias to shared info in native source */
    H5O_shared_t        *shared_dst = (H5O_shared_t *)mesg_dst; /* Alias to shared info in native destination */
    herr_t              ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(oloc_src->file);
    HDassert(oloc_dst->file);
    HDassert(mesg_src);
    HDassert(mesg_dst);
    HDassert(cpy_info);

#ifndef H5O_SHARED_TYPE
#error "Need to define H5O_SHARED_TYPE macro!"
#endif /* H5O_SHARED_TYPE */
#ifndef H5O_SHARED_POST_COPY_FILE
#error "Need to define H5O_SHARED_POST_COPY_FILE macro!"
#endif /* H5O_SHARED_POST_COPY_FILE */

#ifdef H5O_SHARED_POST_COPY_FILE_REAL
    /* Call native message's post copy file callback to copy the message */
    if(H5O_SHARED_POST_COPY_FILE_REAL(oloc_src, mesg_src, oloc_dst, mesg_dst, cpy_info) <0 )
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy native message to another file")
#endif /* H5O_SHARED_POST_COPY_FILE_REAL */

    /* Update shared message after the post copy - will short circuit in
     * production if the DEFER pass determined it will not be shared; debug mode
     * verifies that it is indeed the case */
    if(H5O__shared_post_copy_file(oloc_dst->file, H5O_SHARED_TYPE,
            shared_src, shared_dst, mesg_flags, cpy_info) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to fix shared message in post copy")

#ifdef H5O_SHARED_POST_COPY_FILE_UPD
    /* Call native message's post copy file update callback to update the
     * message */
    if(H5O_SHARED_POST_COPY_FILE_UPD(oloc_src, mesg_src, oloc_dst, mesg_dst, cpy_info) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to update native message")
#endif /* H5O_SHARED_POST_COPY_FILE_UPD */

    /* Make sure that if the the source or destination is committed, both are
     * committed */
    HDassert((shared_src->type == H5O_SHARE_TYPE_COMMITTED)
            == (shared_dst->type == H5O_SHARE_TYPE_COMMITTED));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_SHARED_POST_COPY_FILE() */


/*-------------------------------------------------------------------------
 * Function:    H5O_SHARED_DEBUG
 *
 * Purpose:     Prints debugging info for a potentially shared message.
 *
 * Note:	The actual name of this routine can be different in each source
 *		file that this header file is included in, and must be defined
 *		prior to including this header file.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, February  3, 2007
 *
 *-------------------------------------------------------------------------
 */
static H5_INLINE herr_t
H5O_SHARED_DEBUG(H5F_t *f, const void *_mesg, FILE *stream, int indent,
    int fwidth)
{
    const H5O_shared_t *sh_mesg = (const H5O_shared_t *)_mesg;     /* Pointer to shared message portion of actual message */
    herr_t ret_value = SUCCEED;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

#ifndef H5O_SHARED_TYPE
#error "Need to define H5O_SHARED_TYPE macro!"
#endif /* H5O_SHARED_TYPE */
#ifndef H5O_SHARED_DEBUG
#error "Need to define H5O_SHARED_DEBUG macro!"
#endif /* H5O_SHARED_DEBUG */
#ifndef H5O_SHARED_DEBUG_REAL
#error "Need to define H5O_SHARED_DEBUG_REAL macro!"
#endif /* H5O_SHARED_DEBUG_REAL */

    /* Check for message stored elsewhere */
    if(H5O_IS_STORED_SHARED(sh_mesg->type)) {
        /* Print shared message information */
        if(H5O_shared_debug(sh_mesg, stream, indent, fwidth) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to display shared message info")
    } /* end if */

    /* Call native message's debug callback */
    if(H5O_SHARED_DEBUG_REAL(f, _mesg, stream, indent, fwidth) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_WRITEERROR, FAIL, "unable to display native message info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_SHARED_DEBUG() */

#endif /* H5Oshared_H */

