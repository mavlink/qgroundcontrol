/**
******************************************************************************
*
* @file       providerstrings.h
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
#ifndef PROVIDERSTRINGS_H
#define PROVIDERSTRINGS_H

#include <QString>


namespace core {
    class ProviderStrings
    {
    public:
        ProviderStrings();
        static const QString levelsForSigPacSpainMap[];
        QString GoogleMapsAPIKey;
        // Google version strings
        QString VersionGoogleMap;
        QString VersionGoogleSatellite;
        QString VersionGoogleLabels;
        QString VersionGoogleTerrain;
        QString SecGoogleWord;

        // Google (China) version strings
        QString VersionGoogleMapChina;
        QString VersionGoogleSatelliteChina;
        QString VersionGoogleLabelsChina;
        QString VersionGoogleTerrainChina;

        // Google (Korea) version strings
        QString VersionGoogleMapKorea;
        QString VersionGoogleSatelliteKorea;
        QString VersionGoogleLabelsKorea;

        /// <summary>
        /// Google Maps API generated using http://greatmaps.codeplex.com/
        /// from http://code.google.com/intl/en-us/apis/maps/signup.html
        /// </summary>


        // Yahoo version strings
        QString VersionYahooMap;
        QString VersionYahooSatellite;
        QString VersionYahooLabels;

        // BingMaps
        QString VersionBingMaps;

        // YandexMap
        QString VersionYandexMap;



        /// <summary>
        /// Bing Maps Customer Identification, more info here
        /// http://msdn.microsoft.com/en-us/library/bb924353.aspx
        /// </summary>
        QString BingMapsClientToken;
    };

}
#endif // PROVIDERSTRINGS_H
