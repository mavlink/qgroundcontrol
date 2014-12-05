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

#ifndef QGCTemporaryFile_H
#define QGCTemporaryFile_H

#include <QFile>

/// @file
///     @brief This class mimics QTemporaryFile. We have our own implementation due to the fact that
///				QTemporaryFile implemenation differs cross platform making it unusable for our use-case.
///				Look for bug reports on QTemporaryFile keeping the file locked for details.
///
///     @author Don Gagne <don@thegagnes.com>

class QGCTemporaryFile : public QFile {
    Q_OBJECT
    
public:
	/// @brief Creates a new temp file object. QGC temp files are always created in the
	//			QStandardPaths::TempLocation directory.
	//		@param template Template for file name following QTemporaryFile rules. Template should NOT include
	//							directory path, only file name.
    QGCTemporaryFile(const QString& fileTemplate, QObject* parent = NULL);

	/// @brief Opens the file in ReadWrite mode.
	///		@returns false - open failed
	bool open(OpenMode openMode = ReadWrite);
    
private:
    QString _template;
};


#endif
