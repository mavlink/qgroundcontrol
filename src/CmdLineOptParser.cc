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

#include "CmdLineOptParser.h"

#include <QString>

/// @brief Implements a simple command line parser which sets booleans to true if the option is found.
void ParseCmdLineOptions(int&           argc,                   ///< count of arguments in argv
                         char*          argv[],                 ///< command line arguments
                         CmdLineOpt_t*  prgOpts,                ///< command line options
                         size_t         cOpts,                  ///< count of command line options
                         bool           removeParsedOptions)    ///< true: remove parsed option from argc/argv
{
    // Start with all options off
    for (size_t iOption=0; iOption<cOpts; iOption++) {
        *prgOpts[iOption].optionFound = false;
    }
    
    for (int iArg=1; iArg<argc; iArg++) {
        for (size_t iOption=0; iOption<cOpts; iOption++) {
            bool found = false;
            
            QString arg(argv[iArg]);
            QString optionStr(prgOpts[iOption].optionStr);
            
            if (arg.startsWith(QString("%1:").arg(optionStr), Qt::CaseInsensitive)) {
                found = true;
                if (prgOpts[iOption].optionArg) {
                    *prgOpts[iOption].optionArg = arg.right(arg.length() - (optionStr.length() + 1));
                }
            } else if (arg.compare(optionStr, Qt::CaseInsensitive) == 0) {
                found = true;
            }
            if (found) {
                *prgOpts[iOption].optionFound = true;
                if (removeParsedOptions) {
                    for (int iShift=iArg; iShift<argc-1; iShift++) {
                        argv[iShift] = argv[iShift+1];
                    }
                    argc--;
                    iArg--;
                }
            }
        }
    }
}
