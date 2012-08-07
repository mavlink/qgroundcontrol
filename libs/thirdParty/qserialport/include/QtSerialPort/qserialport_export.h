/*
 * Unofficial Qt Serial Port Library
 *
 * Copyright (c) 2010 Inbiza Systems Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * author labs@inbiza.com
 */

/**
   \file qserialport_export.h

   Preprocessor magic to allow export of library symbols.

   This is strictly internal.

   \note You should not include this header directly from an
   application. You should just use <tt> \#include \<QSerialPort> </tt> instead.
*/

#ifndef TNX_QSERIALPORT_EXPORT_H__
#define TNX_QSERIALPORT_EXPORT_H__

#include <QtGlobal>

#ifdef Q_OS_WIN
# ifndef QSERIALPORT_STATIC
#  ifdef QSERIALPORT_MAKEDLL
#   define TONIX_EXPORT Q_DECL_EXPORT
#  else
#   define TONIX_EXPORT Q_DECL_IMPORT
#  endif
# endif
#endif

#ifndef TONIX_EXPORT
# define TONIX_EXPORT
#endif

#endif // TNX_QSERIALPORT_EXPORT_H__
