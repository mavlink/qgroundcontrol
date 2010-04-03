/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief List of detected objects
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _OBJECTDETECTIONVIEW_H_
#define _OBJECTDETECTIONVIEW_H_

#include <QtGui/QWidget>
#include <QResizeEvent>
#include <QMap>
#include "UASInterface.h"

namespace Ui {
    class ObjectDetectionView;
}

/**
 * @brief Lists the detected objects and their confidence
 */
class ObjectDetectionView : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(ObjectDetectionView)
        public:
            explicit ObjectDetectionView(QString folder="test", QWidget *parent = 0);
    virtual ~ObjectDetectionView();

    /** @brief Resize widget contents */
    void resizeEvent(QResizeEvent * event );

public slots:
    /** @brief Set the UAS this view is currently associated to */
    void setUAS(UASInterface* uas);
    /** @brief Report new detection */
    void newDetection(int uasId, QString patternPath, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, double confidence, bool detected);
    /** @brief Accept an internal action, update name and preview image label */
    void takeAction();

protected:
    virtual void changeEvent(QEvent *e);
    QMap<QString, double>  patternList;  ///< The detected patterns
    QMap<QString, unsigned int> patternCount; ///< Number of detections per pattern
    UASInterface*   uas;                 ///< The monitored UAS
    QString patternFolder;               ///< The base folder where pattern images are stored in
    const QString separator;

private:
    Ui::ObjectDetectionView *m_ui;
};

#endif // _OBJECTDETECTIONVIEW_H_
