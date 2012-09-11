/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief List of detected objects
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Fabian Landau <mavteam@student.ethz.ch>
 *
 */

#ifndef _OBJECTDETECTIONVIEW_H_
#define _OBJECTDETECTIONVIEW_H_

#include <QtGui/QWidget>
#include <QResizeEvent>
#include <QMap>
#include "UASInterface.h"

namespace Ui
{
class ObjectDetectionView;
}

/**
 * @brief Lists the detected objects and their confidence
 */
class ObjectDetectionView : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(ObjectDetectionView)

    struct Pattern {
        Pattern() : name(QString()), confidence(0.0f), count(0) {}
        Pattern(QString name, float confidence) : name(name), confidence(confidence), count(1) {}

        bool operator<(const Pattern& other) const {
            return this->confidence > other.confidence;    // this comparison is intentionally wrong to sort the QList from highest confidence to lowest
        }

        QString name;
        float confidence;
        unsigned int count;
    };

public:
    explicit ObjectDetectionView(QString folder="files/images/patterns", QWidget *parent = 0);
    virtual ~ObjectDetectionView();

    /** @brief Resize widget contents */
    void resizeEvent(QResizeEvent * event );

public slots:
    /** @brief Set the UAS this view is currently associated to */
    void setUAS(UASInterface* uas);
    /** @brief Report new detection */
    void newPattern(int uasId, QString patternPath, float confidence, bool detected);
    void newLetter(int uasId, QString letter, float confidence, bool detected);
    void decreaseLetterTime();
    void updateLetterList();
    void clearLists();
    /** @brief Accept an internal action, update name and preview image label */
    void takeAction();

protected:
    virtual void changeEvent(QEvent *e);
    QMap<QString, Pattern> patternList;  ///< The detected patterns with their confidence and detection count
    QMap<QString, Pattern> letterList;   ///< The detected letters with their confidence and detection count
    QTimer letterTimer;                  ///< A timer to "forget" old letters
    UASInterface* uas;                   ///< The monitored UAS
    QString patternFolder;               ///< The base folder where pattern images are stored in
    const QString separator;

private:
    Ui::ObjectDetectionView *m_ui;
};

#endif // _OBJECTDETECTIONVIEW_H_
