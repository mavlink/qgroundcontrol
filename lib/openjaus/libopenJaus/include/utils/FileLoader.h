/*****************************************************************************
 *  Copyright (c) 2008, University of Florida
 *  All rights reserved.
 *  
 *  This file is part of OpenJAUS.  OpenJAUS is distributed under the BSD 
 *  license.  See the LICENSE file for details.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the University of Florida nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/
// File Name: FileLoader.h
//
// Written By: Jeffrey Kunkle
//
// Version: 3.3.0a
//
// Date: 04/26/2006
//
// Description: 

#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include <vector>
#include <string>
#include <fstream>
#include <math.h>

using std::string;
using std::vector;
using std::ios;
using std::ifstream;

#ifdef WIN32
	#define EXPORT	__declspec(dllexport)
#else
	#define EXPORT
#endif

struct Config_Data_t
{
	string label;
	vector< string > tok;
};

struct Config_File_Data_t
{
	string configheader;
	vector< Config_Data_t > data;
};

class FileLoader
{
private:
	vector< Config_File_Data_t > configFileData;
	int findHeader( string hdr );
	int findLabel( int hdrIndex, string label );
        
public:
	/**
	 * @brief Constructs the File Loader
	 */
	EXPORT FileLoader( );
	EXPORT FileLoader( string filename );

	/**
	 * @brief Deconstructs the File Loader
	 */
	EXPORT ~FileLoader( );

	/**
	 * @brief Loads the configuration file
	 *
	 * @param[in] filename is the name of the configuration file with extension
	 * @return success
	 */
	EXPORT bool load_cfg( string filename );

	/**
	 * @brief Reads a string from the config file
	 *
	 * @param[in] header The data grouping. In the config file, this is enclosed in "[]"
	 * @param[in] label The label of the data being searched for. In the config file, this ends with ":"
	 *
	 * @return The string being searched for or "" if not found
	 */
	EXPORT string GetConfigDataString( string header, string label );

	/**
	 * @brief Reads an int from the config file
	 *
	 * @param[in] header The data grouping. In the config file, this is enclosed in "[]"
	 * @param[in] label The label of the data being searched for. In the config file, this ends with ":"
	 *
	 * @return The int being searched for, -1 if not found, or 0 if a string or char
	 */
	EXPORT int GetConfigDataInt( string header, string label );

	/**
	 * @brief Reads a float from the config file
	 *
	 * @param[in] header The data grouping. In the config file, this is enclosed in "[]"
	 * @param[in] label The label of the data being searched for. In the config file, this ends with ":"
	 *
	 * @return The float being searched for, -1 if not found, or 0 if a string or char
	 */
	EXPORT float GetConfigDataFloat( string header, string label );

	/**
	 * @brief Reads a double from the config file
	 *
	 * @param[in] header The data grouping. In the config file, this is enclosed in "[]"
	 * @param[in] label The label of the data being searched for. In the config file, this ends with ":"
	 *
	 * @return The double being searched for, -1 if not found, or 0 if a string or char
	 */
	EXPORT double GetConfigDataDouble( string header, string label );

	/**
	 * @brief Reads a vector from the config file
	 *
	 * @param[in] header The data grouping. In the config file, this is enclosed in "[]"
	 * @param[in] label The label of the data being searched for. In the config file, this ends with ":"
	 *
	 * @return A pointer to the vector being searched for or NULL if nothing found
	 */
	EXPORT vector< string >* GetConfigDataVector( string header, string label );
	
	/**
	 * @brief Reads a boolean from the config file
	 *
	 * @param[in] header The data grouping. In the config file, this is enclosed in "[]"
	 * @param[in] label The label of the data being searched for. In the config file, this ends with ":"
	 *
	 * @return true if "true" (compared case insensitve) is found, false otherwise
	 */
	EXPORT bool GetConfigDataBool( string header, string label );
};
#endif
