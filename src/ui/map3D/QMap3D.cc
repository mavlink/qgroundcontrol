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
 *   @brief Definition of the class QMap3D.
 *
 *   @author James Goppert <james.goppert@gmail.com>
 *
 */

#include "QMap3D.h"
#include <osgEarthUtil/EarthManipulator>
#include <QFileDialog>

QMap3D::QMap3D(QWidget * parent, const char * name, WindowFlags f) :
        QWidget(parent,f)
{
    setupUi(this);
    graphicsView->setCameraManipulator(new osgEarthUtil::EarthManipulator);
    graphicsView->setSceneData(new osg::Group);
    graphicsView->updateCamera();
    show();
}

QMap3D::~QMap3D()
{
}

void QMap3D::on_pushButton_map_clicked()
{
    QString mapName = QFileDialog::getOpenFileName(this, tr("Select an OsgEarth map file"),
        QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("OsgEarth file (*.earth);;"));
    graphicsView->getSceneData()->asGroup()->addChild(osgDB::readNodeFile(mapName.toStdString()));
    graphicsView->updateCamera();  
}

void QMap3D::on_pushButton_vehicle_clicked()
{
    QString vehicleName = QFileDialog::getOpenFileName(this, tr("Select a 3D model for your vehicle"),
        QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("OsgEarth file (*.osg, *.ac, *.3ds);;"));
    graphicsView->getSceneData()->asGroup()->addChild(osgDB::readNodeFile(vehicleName.toStdString()));
    graphicsView->updateCamera();
}
