/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Dialog to configure RC to parameter mapping
///     @author Thomas Gubler <thomasgubler@gmail.com>

#pragma once

#include <QDialog>
#include <QThread>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "MultiVehicleManager.h"

namespace Ui {
class QGCMapRCToParamDialog;
}



class QGCMapRCToParamDialog : public QDialog
{
    Q_OBJECT
    QThread paramLoadThread;

public:
    explicit QGCMapRCToParamDialog(QString param_id, UASInterface *mav, MultiVehicleManager* multiVehicleManager, QWidget *parent = 0);
    ~QGCMapRCToParamDialog();

signals:
    void mapRCToParamDialogResult(QString param_id, float scale, float value0,
            quint8 param_rc_channel_index, float valueMin, float valueMax);

public slots:
    void accept();

protected:
    // void showEvent(QShowEvent * event );
    QString param_id;
    UASInterface *mav;

private slots:
    void _parameterUpdated(QVariant value);

private:
    MultiVehicleManager*        _multiVehicleManager;
    Ui::QGCMapRCToParamDialog*  ui;
};

