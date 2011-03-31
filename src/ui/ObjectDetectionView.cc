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
 *   @author Fabian Landau <mavteam@student.ethz.ch>
 *
 */

#include <QListView>
#include <QPixmap>
#include "ObjectDetectionView.h"
#include "ui_ObjectDetectionView.h"
#include "UASManager.h"
#include "GAudioOutput.h"

#include <QDebug>
#include <QMap>

ObjectDetectionView::ObjectDetectionView(QString folder, QWidget *parent) :
    QWidget(parent),
    patternList(),
    letterList(),
    letterTimer(),
    uas(NULL),
    patternFolder(folder),
    separator(" "),
    m_ui(new Ui::ObjectDetectionView)
{
    m_ui->setupUi(this);
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    letterTimer.start(1000);
    connect(&letterTimer, SIGNAL(timeout()), this, SLOT(decreaseLetterTime()));
    connect(m_ui->clearButton, SIGNAL(clicked()), this, SLOT(clearLists()));

    this->setVisible(false);
}

ObjectDetectionView::~ObjectDetectionView()
{
    delete m_ui;
}

void ObjectDetectionView::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ObjectDetectionView::setUAS(UASInterface* uas)
{
    if (this->uas != NULL) {
        disconnect(this->uas, SIGNAL(patternDetected(int, QString, float, bool)), this, SLOT(newPattern(int, QString, float, bool)));
        disconnect(this->uas, SIGNAL(letterDetected(int, QString, float, bool)), this, SLOT(newLetter(int, QString, float, bool)));
    }

    this->uas = uas;
    connect(uas, SIGNAL(patternDetected(int, QString, float, bool)), this, SLOT(newPattern(int, QString, float, bool)));
    connect(uas, SIGNAL(letterDetected(int, QString, float, bool)), this, SLOT(newLetter(int, QString, float, bool)));
}

void ObjectDetectionView::newPattern(int uasId, QString patternPath, float confidence, bool detected)
{
    if (detected) {
        if (!patternList.contains(patternPath)) {
            // Emit audio message on detection
            if (detected) GAudioOutput::instance()->say("System " + QString::number(uasId) + " detected pattern " + QString(patternPath.split("/", QString::SkipEmptyParts).last()).split(".", QString::SkipEmptyParts).first());

            patternList.insert(patternPath, Pattern(patternPath, confidence));
        } else {
            Pattern pattern = patternList.value(patternPath);
            if (confidence > pattern.confidence)
                pattern.confidence = confidence;
            ++pattern.count;
            patternList.insert(patternPath, pattern);
        }

        // set list items
        QList<Pattern> templist;
        foreach (Pattern pattern, patternList)
        templist.push_back(pattern);
        qSort(templist);
        m_ui->listWidget->clear();
        foreach (Pattern pattern, templist)
        m_ui->listWidget->addItem(pattern.name + separator + "(" + QString::number(pattern.count) + ")" + separator + QString::number(pattern.confidence));

        // load image
        QString filePath = patternFolder + "/" + patternPath.split("/", QString::SkipEmptyParts).last();
        QPixmap image = QPixmap(filePath);
        if (image.width() > image.height())
            image = image.scaledToWidth(m_ui->imageLabel->width());
        else
            image = image.scaledToHeight(m_ui->imageLabel->height());
        m_ui->imageLabel->setPixmap(image);

        // set textlabel
        QString patternName = patternPath.split("/", QString::SkipEmptyParts).last(); // Remove preceding folder names
        patternName = patternName.split(".", QString::SkipEmptyParts).first(); // Remove file ending
        m_ui->nameLabel->setText("Pattern: " + patternName);
    }
}

void ObjectDetectionView::newLetter(int uasId, QString letter, float confidence, bool detected)
{
    Q_UNUSED(confidence);

    if (detected) {
        if (!letterList.contains(letter)) {
            // Emit audio message on detection
            if (detected) GAudioOutput::instance()->say("System " + QString::number(uasId) + " detected letter " + letter);

            letterList.insert(letter, Pattern(letter, 0));
        } else {
            Pattern pattern = letterList.value(letter);
            pattern.confidence = 0;
            ++pattern.count;
            letterList.insert(letter, pattern);
        }

        updateLetterList();

        // display letter
        m_ui->letterLabel->setText(letter);

        // set textlabel
        m_ui->nameLabel->setText("Letter: " + letter);
    }
}

void ObjectDetectionView::decreaseLetterTime()
{
    foreach (Pattern pattern, letterList) {
        pattern.confidence -= 1;
        letterList.insert(pattern.name, pattern);
    }

    updateLetterList();
}

void ObjectDetectionView::updateLetterList()
{
    // set list items
    QList<Pattern> templist;
    foreach (Pattern pattern, letterList)
    templist.push_back(pattern);
    qSort(templist);
    m_ui->letterListWidget->clear();
    foreach (Pattern pattern, templist)
    m_ui->letterListWidget->addItem(pattern.name + separator + "(" + QString::number(pattern.count) + ")" + separator + QString::number(pattern.confidence));
}

void ObjectDetectionView::clearLists()
{
    patternList.clear();
    letterList.clear();

    m_ui->listWidget->clear();
    m_ui->letterListWidget->clear();

    m_ui->imageLabel->clear();;
    m_ui->letterLabel->clear();
    m_ui->nameLabel->clear();
}

void ObjectDetectionView::takeAction()
{
    QAction* act = dynamic_cast<QAction*>(sender());
    if (act) {
        QString patternPath = act->text().trimmed().split(separator, QString::SkipEmptyParts).first(); // Remove additional information
        QString patternName = patternPath.split("//", QString::SkipEmptyParts).last(); // Remove preceding folder names
        patternName = patternName.split(".", QString::SkipEmptyParts).first(); // Remove file ending

        // Set name and label
        m_ui->nameLabel->setText(patternName);
        m_ui->imageLabel->setPixmap(act->icon().pixmap(64, 64));
    }
}

void ObjectDetectionView::resizeEvent(QResizeEvent * event )
{
    if (event->isAccepted()) {
        // Enforce square shape of image label
        m_ui->imageLabel->resize(m_ui->imageLabel->width(), m_ui->imageLabel->width());
    }
}

