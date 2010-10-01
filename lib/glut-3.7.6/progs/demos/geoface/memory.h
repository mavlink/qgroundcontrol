/* ==========================================================================
                                MEMORY_H                              
=============================================================================

      AUTHOR: Keith Waters
      DATE  : Tue Jan  7 10:16:52 EST 1992

      SYNOPSIS
         Memory macros.

      DESCRIPTION
         Simple memory macros for allocation.

============================================================================ */


#ifndef	MEMORY_H
#define	MEMORY_H


#define	_new(t)			((t*)malloc(sizeof(t)))
#define	_new_array(t, n)	((t*)malloc(sizeof(t) * (n)))
#define	_resize_array(a, t, n)	((t*)realloc((a), sizeof(t) * (n)))
#define _size_array(a,t,n0,n1)  a = (n0 == 0 ? _new_array(t,n1) : \
                                               _resize_array(a,t,n1))
#define	_delete(object)		((void)(((object)!=NULL) ? \
				    free((char*)(object)),(object)=NULL : 0))
#endif	/* _MEMORY_H */

