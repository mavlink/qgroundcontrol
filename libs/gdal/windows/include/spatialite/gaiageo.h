/*
 gaiageo.h -- Gaia common support for geometries
  
 version 4.3, 2015 June 29

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2015
the Initial Developer. All Rights Reserved.

Contributor(s):
Klaus Foerster klaus.foerster@svg.cc

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/


/**
 \file gaiageo.h

 Geometry handling functions and constants 
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/* stdio.h included for FILE objects. */
#include <stdio.h>
#ifdef DLL_EXPORT
#define GAIAGEO_DECLARE __declspec(dllexport)
#else
#define GAIAGEO_DECLARE extern
#endif
#endif

#ifndef _GAIAGEO_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GAIAGEO_H
#endif

#include "gg_const.h"
#include "gg_structs.h"
#include "gg_core.h"
#include "gg_mbr.h"
#include "gg_formats.h"
#include "gg_dynamic.h"
#include "gg_advanced.h"
#include "gg_xml.h"

#endif /* _GAIAGEO_H */
