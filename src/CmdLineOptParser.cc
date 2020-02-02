/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
/// @brief Command line option parser implementation
/// @author Don Gagne <don@thegagnes.com>

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
