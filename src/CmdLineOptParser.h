/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @brief Command line option parser
///
///     @author Don Gagne <don@thegagnes.com>

#ifndef CMDLINEOPTPARSER_H
#define CMDLINEOPTPARSER_H

#include <QString>
#include <cstring>

/// @brief Structure used to pass command line options to the ParseCmdLineOptions function.
typedef struct {
    const char* optionStr;      ///< command line option, for example "--foo"
    bool*       optionFound;    ///< if option is found this variable will be set to true
    QString*    optionArg;      ///< Option has additional argument, form is option:arg
} CmdLineOpt_t;

void ParseCmdLineOptions(int&           argc,
                         char*          argv[],
                         CmdLineOpt_t*  prgOpts,
                         size_t         cOpts,
                         bool           removeParsedOptions);

#endif
