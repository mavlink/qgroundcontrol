/**
******************************************************************************
*
* @file       languagetype.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
* 
*****************************************************************************/
/* 
* This program is free software; you can redistribute it and/or modify 
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 3 of the License, or 
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
* for more details.
* 
* You should have received a copy of the GNU General Public License along 
* with this program; if not, write to the Free Software Foundation, Inc., 
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "languagetype.h"



namespace core {
    LanguageType::LanguageType()
    {
        list
                <<"ar"
                <<"bg"
                <<"bn"
                <<"ca"
                <<"cs"
                <<"da"
                <<"de"
                <<"el"
                <<"en"
                <<"en-AU"
                <<"en-GB"
                <<"es"
                <<"eu"
                <<"fi"
                <<"fil"
                <<"fr"
                <<"gl"
                <<"gu"
                <<"hi"
                <<"hr"
                <<"hu"
                <<"id"
                <<"it"
                <<"iw"
                <<"ja"
                <<"kn"
                <<"ko"
                <<"lt"
                <<"lv"
                <<"ml"
                <<"mr"
                <<"nl"
                <<"nn"
                <<"no"
                <<"or"
                <<"pl"
                <<"pt"
                <<"pt-BR"
                <<"pt-PT"
                <<"rm"
                <<"ro"
                <<"ru"
                <<"sk"
                <<"sl"
                <<"sr"
                <<"sv"
                <<"ta"
                <<"te"
                <<"th"
                <<"tr"
                <<"uk"
                <<"vi"
                <<"zh-CN"
                <<"zh-TW";

    }
    QString LanguageType::toShortString(Types type)
    {
        return list[type];
    }
    LanguageType::~LanguageType()
    {
        list.clear();
    }

}
