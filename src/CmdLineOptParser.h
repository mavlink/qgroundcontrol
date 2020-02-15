/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
/// @brief  Command line option parser declaration
/// @author Don Gagne <don@thegagnes.com>

#pragma once

#include <QString>
#include <cstring>

/// @brief Structure used to pass command line options to the ParseCmdLineOptions function.
typedef struct {
    const char* optionStr;      ///< Command line option, for example "--foo"
    bool*       optionFound;    ///< If option is found this variable will be set to true
    QString*    optionArg;      ///< Option has additional argument, form is option:arg
} CmdLineOpt_t;

void ParseCmdLineOptions(int&           argc,
                         char*          argv[],
                         CmdLineOpt_t*  prgOpts,
                         size_t         cOpts,
                         bool           removeParsedOptions);

