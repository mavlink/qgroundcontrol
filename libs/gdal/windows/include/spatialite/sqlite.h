/* 
 sqlite.h -- supporting SQLite headers in a flexible way
  
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
 
Portions created by the Initial Developer are Copyright (C) 2013-2015
the Initial Developer. All Rights Reserved.

Contributor(s):
Pepijn Van Eeckhoudt <pepijnvaneeckhoudt@luciad.com>

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

#ifndef _SPATIALITE_SQLITE_H
#define _SPATIALITE_SQLITE_H

#ifdef LOADABLE_EXTENSION	/* loadable-extension only */
#ifdef SPL_AMALGAMATION		/* spatialite-amalgamation */
#include <spatialite/sqlite3ext.h>
#else
#include <sqlite3ext.h>
#endif
/* We can't use SQLITE_EXTENSION_INIT1 as this is an intializer in recent version of sqlite */
extern const sqlite3_api_routines *sqlite3_api;
#else /* ordinary lib */
#ifdef SPL_AMALGAMATION		/* spatialite-amalgamation */
#include <spatialite/sqlite3.h>
#else
#include <sqlite3.h>
#endif
#endif

#ifndef SQLITE_DETERMINISTIC
/* probably SQLite < 3.8.3 - attempting to fix */
#define SQLITE_DETERMINISTIC	SQLITE_UTF8
#endif

#endif
