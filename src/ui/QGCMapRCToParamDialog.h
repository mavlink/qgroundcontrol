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

/// @file
///     @brief Dialog to configure RC to paramter mapping
///     @author Thomas Gubler <thomasgubler@gmail.com>

#ifndef QGCMAPRCTOPARAMDIALOG_H
#define QGCMAPRCTOPARAMDIALOG_H

#include <QDialog>
#include <QThread>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

namespace Ui {
class QGCMapRCToParamDialog;
}


class ParamLoader : public QObject
{
    Q_OBJECT

public:
    ParamLoader(QString paramName, UASInterface* uas, QObject* parent = NULL);

public slots:
    void load();

signals:
    void paramLoaded(bool success, float value, QString message = "");
    void correctParameterChanged();
    
private slots:
    void _parameterUpdated(QVariant value);

private:
    UASInterface*       _uas;
    AutoPilotPlugin*    _autopilot;
    QString             _paramName;
    bool                _paramReceived;
};

class QGCMapRCToParamDialog : public QDialog
{
    Q_OBJECT
    QThread paramLoadThread;

public:
    explicit QGCMapRCToParamDialog(QString param_id,
            UASInterface *mav, QWidget *parent = 0);
    ~QGCMapRCToParamDialog();

signals:
    void mapRCToParamDialogResult(QString param_id, float scale, float value0,
            quint8 param_rc_channel_index, float valueMin, float valueMax);
    void refreshParam();

public slots:
    void accept();
    void paramLoaded(bool success, float value, QString message);

protected:
    // void showEvent(QShowEvent * event );
    QString param_id;
    UASInterface *mav;

private:
    Ui::QGCMapRCToParamDialog *ui;
};

#endif // QGCMAPRCTOPARAMDIALOG_H
