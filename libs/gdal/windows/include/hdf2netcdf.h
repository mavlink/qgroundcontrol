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

#include "h4config.h"
#include "H4api_adpt.h"
/* If we disable the HDF version of the netCDF API (ncxxx interface)
   (--disable-netcdf configure flag; the old way was to use -DHAVE_NETCDF compilation flag)
 ) we need to rename all the relevant function names 
   In this version we exclude renaming the netCDF fortran API so 
   the MFHDF side must be compilied without fortran support. */
#ifndef H4_HAVE_NETCDF
#define  HNAME(x)  sd_##x     /* pre-append 'sd_' to all netCDF fcn names */
#else /* !H4_HAVE_NETCDF i.e NOT USING HDF NETCDF */
#define  HNAME(x)   x
#endif /* H4_HAVE_NETCDF i.e. USING HDF NETCDF */

/* If using the real netCDF library and API (use --disable-netcdf configure flag))
   need to mangle the HDF versions of netCDF API function names 
   to not conflict w/ oriinal netCDF ones */
#ifndef H4_HAVE_NETCDF
#define ncerr     HNAME(ncerr)
#define ncopts    HNAME(ncopts)
#define nccreate  HNAME(nccreate)
#define ncopen    HNAME(ncopen)
#define ncredef   HNAME(ncredef)
#define ncendef   HNAME(ncendef)
#define ncclose   HNAME(ncclose)
#define ncinquire HNAME(ncinquire)
#define ncsync    HNAME(ncsync)
#define ncabort   HNAME(ncabort)
#define ncdimdef  HNAME(ncdimdef)
#define ncdimid   HNAME(ncdimid)
#define ncdiminq  HNAME(ncdiminq)
#define ncdimrename HNAME(ncdimrename)
#define ncvardef  HNAME(ncvardef)
#define ncvarid   HNAME(ncvarid)
#define ncvarinq  HNAME(ncvarinq)
#define ncvarput1 HNAME(ncvarput1)
#define ncvarget1 HNAME(ncvarget1)
#define ncvarput  HNAME(ncvarput)
#define ncvarget  HNAME(ncvarget)
#define ncvarputs HNAME(ncvarputs)
#define ncvargets HNAME(ncvargets)
#define ncvarputg HNAME(ncvarputg)
#define ncvargetg HNAME(ncvargetg)
#define ncvarrename HNAME(ncvarrename)
#define ncattput  HNAME(ncattput)
#define ncattinq  HNAME(ncattinq)
#define ncattget  HNAME(ncattget)
#define ncattcopy HNAME(ncattcopy)
#define ncattname HNAME(ncattname)
#define ncattrename HNAME(ncattrename)
#define ncattdel  HNAME(ncattdel)
#define nctypelen HNAME(nctypelen)
#define ncsetfill HNAME(ncsetfill)
#define ncrecinq  HNAME(ncrecinq)
#define ncrecget  HNAME(ncrecget)
#define ncrecput  HNAME(ncrecput)
#define ncnobuf   HNAME(ncnobuf) /* no prototype for this one */

#endif /* !H4_HAVE_NETCDF i.e NOT USING HDF version of netCDF API */ 
