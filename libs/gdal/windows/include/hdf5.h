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
 * This is the main public HDF5 include file.  Put further information in
 * a particular header file and include that here, don't fill this file with
 * lots of gunk...
 */
#ifndef _HDF5_H
#define _HDF5_H

#include "H5public.h"
#include "H5Apublic.h"          /* Attributes                               */
#include "H5ACpublic.h"         /* Metadata cache                           */
#include "H5Dpublic.h"          /* Datasets                                 */
#include "H5Epublic.h"          /* Errors                                   */
#include "H5Fpublic.h"          /* Files                                    */
#include "H5FDpublic.h"         /* File drivers                             */
#include "H5Gpublic.h"          /* Groups                                   */
#include "H5Ipublic.h"          /* ID management                            */
#include "H5Lpublic.h"          /* Links                                    */
#include "H5MMpublic.h"         /* Memory management                        */
#include "H5Opublic.h"          /* Object headers                           */
#include "H5Ppublic.h"          /* Property lists                           */
#include "H5PLpublic.h"         /* Plugins                                  */
#include "H5Rpublic.h"          /* References                               */
#include "H5Spublic.h"          /* Dataspaces                               */
#include "H5Tpublic.h"          /* Datatypes                                */
#include "H5Zpublic.h"          /* Data filters                             */

/* Predefined file drivers */
#include "H5FDcore.h"           /* Files stored entirely in memory              */
#include "H5FDdirect.h"         /* Linux direct I/O                             */
#include "H5FDfamily.h"         /* File families                                */
#include "H5FDlog.h"            /* sec2 driver with I/O logging (for debugging) */
#include "H5FDmpi.h"            /* MPI-based file drivers                       */
#include "H5FDmulti.h"          /* Usage-partitioned file family                */
#include "H5FDsec2.h"           /* POSIX unbuffered file I/O                    */
#include "H5FDstdio.h"          /* Standard C buffered I/O                      */
#ifdef H5_HAVE_WINDOWS
#include "H5FDwindows.h"        /* Win32 I/O                                    */
#endif

#endif
