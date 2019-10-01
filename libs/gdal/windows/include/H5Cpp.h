// C++ informative line for the emacs editor: -*- C++ -*-
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

#ifndef __H5Cpp_H
#define __H5Cpp_H

#include "H5Include.h"
#include "H5Exception.h"
#include "H5IdComponent.h"
#include "H5DataSpace.h"
#include "H5PropList.h"
#include "H5FaccProp.h"
#include "H5FcreatProp.h"
#include "H5OcreatProp.h"
#include "H5DcreatProp.h"
#include "H5DxferProp.h"
#include "H5LcreatProp.h"
#include "H5LaccProp.h"
#include "H5DaccProp.h"
#include "H5Location.h"
#include "H5Object.h"
#include "H5AbstractDs.h"
#include "H5Attribute.h"
#include "H5DataType.h"
#include "H5AtomType.h"
#include "H5PredType.h"
#include "H5EnumType.h"
#include "H5IntType.h"
#include "H5FloatType.h"
#include "H5StrType.h"
#include "H5CompType.h"
#include "H5ArrayType.h"
#include "H5VarLenType.h"
#include "H5DataSet.h"
#include "H5CommonFG.h"
#include "H5Group.h"
#include "H5File.h"
#include "H5Library.h"

/* Some C++ compilers do not have offsetof macro; define to bypass the problem
   - BMR- -EIP- 2007/08/01
*/
#ifndef H5_CXX_HAVE_OFFSETOF
#ifdef HOFFSET
   #undef HOFFSET
#endif
#define HOFFSET(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#endif // __H5Cpp_H
