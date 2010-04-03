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
// File Name: FileLoader.cpp
//
// Written By: Jeffrey Kunkle
//
// Version: 3.3.0a
//
// Date: 04/26/2006
//
// Modifications:
// 	Date/Author
// 	Description
// 
// @par 04-26-2006 Jeff Kunkle
// - Initial Creation
// 
// @par 12-08-2006 Jeff Kunkle
// - Updated to Visual Studio 2005.
// 
// @par 10-23-2007 Danny Kent
// - Updated for use in the NodeManager project

#include "utils/FileLoader.h"

#ifndef WIN32
	#include <cstdlib>
	#include <string.h>
	#define stricmp(x,y) strcasecmp(x,y)
	#define strtok_s(x,y,z) strtok_r(x,y,z)
#else
	#define stricmp(x,y) _stricmp(x,y)
#endif


FileLoader::FileLoader(){}
FileLoader::FileLoader( string filename ){ load_cfg( filename); }

FileLoader::~FileLoader(){}

bool FileLoader::load_cfg( string filename )
{
	ifstream cfg_file( filename.c_str(), ios::in );
	
    if( cfg_file.fail() )
    {
		//DebugUtil::error( "FileLoader: load_cfg: Unable to open file: \"%s\"\n", filename.c_str() );
        cfg_file.close();
		return false;
    }

    char buffer[256];
    char *token, *next_token;
    
    Config_File_Data_t tmp;
    Config_Data_t tmpInfo;
    tmp.configheader = "";
	
   while( cfg_file.getline( buffer, sizeof( buffer ) - 1 ) )
    {
       token = strtok_s( buffer, ": ", &next_token );
		if ( token == NULL || token[0] == '\r' ); // This is a blank line
		else if( token[0] == ';' ); // This is a comment
		else if( token[0] == '/' ); // This is a comment
		else if( token[0] == '*' ); // This is a comment
		else if( token[0] == '#' ); // This is a comment
		else if( token[0] == '[' ) // This is a label
		{
			if( tmp.configheader != "" ){ configFileData.push_back( tmp ); }
			
			token = strtok_s( token, "[]", &next_token );
			tmp.configheader = token;
			tmp.data.clear();
		}
        else
        {
			tmpInfo.label = token;
		    while( token != NULL )
		    {
			    token = strtok_s( NULL, " ,\r\t", &next_token );
			    if( token != NULL )
				{
					tmpInfo.tok.push_back( token );
				}
			}
			tmp.data.push_back( tmpInfo );
            tmpInfo.label = "";
            tmpInfo.tok.clear();
        }

    }

	if( tmp.configheader != "" ){ configFileData.push_back( tmp ); }
    cfg_file.close();
	
    return true;
}


string FileLoader::GetConfigDataString( string header, string label )
{
    int hdr_index = findHeader( header );
    if( hdr_index == -1 ){ return ""; }

    int lbl_index = findLabel( hdr_index, label );
    if( lbl_index == -1 ){ return ""; }
    
    if( configFileData[ hdr_index ].data[ lbl_index ].tok.empty() ){ return ""; }
    
	return configFileData[ hdr_index ].data[ lbl_index ].tok[0];
}

int FileLoader::GetConfigDataInt( string header, string label )
{
	string tmp = GetConfigDataString( header, label );
	return ( tmp != "" ? atoi( tmp.c_str() ) : -1  );
}

float FileLoader::GetConfigDataFloat( string header, string label )
{
	string tmp = GetConfigDataString( header, label );
	return ( tmp != "" ? (float)atof( tmp.c_str() ) : -1.0f  );
}

double FileLoader::GetConfigDataDouble( string header, string label )
{
	string tmp = GetConfigDataString( header, label );
	return ( tmp != "" ? atof( tmp.c_str() ) : -1  );
}
bool FileLoader::GetConfigDataBool( string header, string label )
{
	string tmp = GetConfigDataString( header, label );
	return ( stricmp( tmp.c_str(), "true" ) == 0 ? true : false  );
}
vector< string >* FileLoader::GetConfigDataVector( string header, string label )
{
    int hdr_index = findHeader( header );
    if( hdr_index == -1 ){ return NULL; }
    
    int lbl_index = findLabel( hdr_index, label );
    if( lbl_index == -1 ){ return NULL; }

    vector< string > *rtnData = new vector< string >();

    int datasize = (int)configFileData[ hdr_index ].data[ lbl_index ].tok.size();

 	for( int i=0; i<datasize; i++ )
    {
		string tmp = configFileData[ hdr_index ].data[ lbl_index ].tok[i];
	    rtnData->push_back( tmp );
	}
	
	return rtnData;
}

int FileLoader::findHeader( string hdr )
{
	int rtn = 0;
    vector< Config_File_Data_t >::iterator findHead = configFileData.begin();

    while( findHead != configFileData.end() && ( *findHead ).configheader != hdr )
    {
		findHead++;
		rtn++;
	}
	if( rtn < (int)configFileData.size() ){ return rtn; }
	else{ return -1; }
}


int FileLoader::findLabel( int hdrIndex, string label )
{
	int rtn = 0;
    vector< Config_Data_t >::iterator findLabel = configFileData[hdrIndex].data.begin();
	
    while( findLabel != configFileData[hdrIndex].data.end() && ( *findLabel ).label != label )
	{
		findLabel++;
		rtn++;
	}

	if( rtn < (int)configFileData[hdrIndex].data.size() ){ return rtn; }
	else{ return -1; }
}
