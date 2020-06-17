/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
    QGCTemporaryFile(const QString& fileTemplate, QObject* parent = nullptr);

	/// @brief Opens the file in ReadWrite mode.
	///		@returns false - open failed
	bool open(OpenMode openMode = ReadWrite);
    
private:
    QString _template;
};


#endif
