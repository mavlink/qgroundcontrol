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

#include <QListView>
#include <QPixmap>
#include "ObjectDetectionView.h"
#include "ui_ObjectDetectionView.h"
#include "UASManager.h"
#include "GAudioOutput.h"

#include <QDebug>

#include "MG.h"

ObjectDetectionView::ObjectDetectionView(QString folder, QWidget *parent) :
        QWidget(parent),
        patternList(),
        patternCount(),
        uas(NULL),
        patternFolder(folder),
        separator(" "),
        m_ui(new Ui::ObjectDetectionView)
{
    m_ui->setupUi(this);
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
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
    //if (this->uas == NULL && uas != NULL)
    //{
    this->uas = uas;
    connect(uas, SIGNAL(detectionReceived(int, QString, int, int, int, int, int, int, int, int, double, bool)), this, SLOT(newDetection(int,QString,int,int,int,int,int,int,int,int,double,bool)));
    //}
}

void ObjectDetectionView::newDetection(int uasId, QString patternPath, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, double confidence, bool detected)
{
    Q_UNUSED(x1);
    Q_UNUSED(y1);
    Q_UNUSED(x2);
    Q_UNUSED(y2);
    Q_UNUSED(x3);
    Q_UNUSED(y3);
    Q_UNUSED(x4);
    Q_UNUSED(y4);
    if (detected)
    {
        if (patternList.contains(patternPath))
        {
            //qDebug() << "REDETECTED";

            QList<QAction*> actions = m_ui->listWidget->actions();
            // Find action and update it
            foreach (QAction* act, actions)
            {
                qDebug() << "ACTION";
                if (act->text().trimmed().split(separator).first() == patternPath)
                {
                    int count = patternCount.value(patternPath);
                    patternCount.insert(patternPath, count);
                    act->setText(patternPath + separator + "(#" + QString::number(count) + ")" + separator + QString::number(confidence));
                }
            }
            QString filePath = MG::DIR::getSupportFilesDirectory() + "/" + patternFolder + "/" + patternPath.split("/").last();
            qDebug() << "Loading:" << filePath;
            QPixmap image = QPixmap(filePath);
            image = image.scaledToWidth(m_ui->imageLabel->width());
            m_ui->imageLabel->setPixmap(image);
            QString patternName = patternPath.split("//").last(); // Remove preceding folder names
            patternName = patternName.split(".").first(); // Remove file ending

            // Set name and label
            m_ui->nameLabel->setText(patternName);
        }
        else
        {
            // Emit audio message on detection
            if (detected) GAudioOutput::instance()->say("System " + QString::number(uasId) + " detected pattern " + QString(patternPath.split("/").last()).split(".").first());

            patternList.insert(patternPath, confidence);
            patternCount.insert(patternPath, 1);

            QString filePath = MG::DIR::getSupportFilesDirectory() + "/" + patternFolder + "/" + patternPath.split("/").last();

            qDebug() << "Loading:" << filePath;
            QPixmap image = QPixmap(filePath);
            QIcon ico(image);
            QAction* act = new QAction(ico, patternPath + separator + "(#" + QString::number(1) + ")" + separator + QString::number(confidence), this);
            connect(act, SIGNAL(triggered()), this, SLOT(takeAction()));
            //m_ui->listWidget->addAction(act);
            m_ui->listWidget->addItem(patternPath + separator + "(#" + QString::number(1) + ")" + separator + QString::number(confidence));
            //m_ui->listWidget->addItem(patternPath + " " + QString::number(confidence));
            image = image.scaledToWidth(m_ui->imageLabel->width());
            m_ui->imageLabel->setPixmap(image);
            QString patternName = patternPath.split("//").last(); // Remove preceding folder names
            patternName = patternName.split(".").first(); // Remove file ending

            // Set name and label
            m_ui->nameLabel->setText(patternName);
            qDebug() << "IMAGE SET" << patternFolder + "/" + patternPath;
        }
    }
}

void ObjectDetectionView::takeAction()
{
    QAction* act = dynamic_cast<QAction*>(sender());
    if (act)
    {
        QString patternPath = act->text().trimmed().split(separator).first(); // Remove additional information
        QString patternName = patternPath.split("//").last(); // Remove preceding folder names
        patternName = patternName.split(".").first(); // Remove file ending

        // Set name and label
        m_ui->nameLabel->setText(patternName);
        m_ui->imageLabel->setPixmap(act->icon().pixmap(64, 64));
    }
}

void ObjectDetectionView::resizeEvent(QResizeEvent * event )
{
    if (event->isAccepted())
    {
        // Enforce square shape of image label
        m_ui->imageLabel->resize(m_ui->imageLabel->width(), m_ui->imageLabel->width());
    }
}

