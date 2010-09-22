#include "RadioCalibrationWindow.h"

RadioCalibrationWindow::RadioCalibrationWindow(QWidget *parent) :
        QWidget(parent, Qt::Window),
        radio(new RadioCalibrationData())
{
    QGridLayout *grid = new QGridLayout();

    aileron = new AirfoilServoCalibrator(AirfoilServoCalibrator::AILERON);
    grid->addWidget(aileron, 0, 0, 1, 1, Qt::AlignTop);

    elevator = new AirfoilServoCalibrator(AirfoilServoCalibrator::ELEVATOR);
    grid->addWidget(elevator, 0, 1, 1, 1, Qt::AlignTop);

    rudder = new AirfoilServoCalibrator(AirfoilServoCalibrator::RUDDER);
    grid->addWidget(rudder, 0, 2, 1, 1, Qt::AlignTop);

    gyro = new SwitchCalibrator(tr("Gyro Mode/Gain"));
    grid->addWidget(gyro, 0, 3, 1, 1, Qt::AlignTop);


    pitch = new CurveCalibrator(tr("Collective Pitch"));
    grid->addWidget(pitch, 1, 0, 1, 2);

    throttle = new CurveCalibrator(tr("Throttle"));
    grid->addWidget(throttle, 1, 2, 1, 2);

    /* Buttons for loading/transmitting calibration data */
    QHBoxLayout *hbox = new QHBoxLayout();
    QPushButton *load = new QPushButton(tr("Load File"));
    QPushButton *save = new QPushButton(tr("Save File"));
    QPushButton *transmit = new QPushButton(tr("Transmit to UAV"));
    QPushButton *get = new QPushButton(tr("Get from UAV"));
    hbox->addWidget(load);
    hbox->addWidget(save);
    hbox->addWidget(transmit);
    hbox->addWidget(get);
    grid->addLayout(hbox, 2, 0, 1, 4);
    this->setLayout(grid);

    connect(load, SIGNAL(clicked()), this, SLOT(loadFile()));
    connect(save, SIGNAL(clicked()), this, SLOT(saveFile()));
    connect(transmit, SIGNAL(clicked()), this, SLOT(send()));
    connect(get, SIGNAL(clicked()), this, SLOT(request()));


    setUASId(0);
}

void RadioCalibrationWindow::setChannel(int ch, float raw, float normalized)
{
    /** this expects a particular channel to function mapping
       \todo allow run-time channel mapping
       */
    switch (ch)
    {
    case 0:
        aileron->channelChanged(raw);
        break;
    case 1:
        elevator->channelChanged(raw);
        break;
    case 2:
        throttle->channelChanged(raw);
        break;
    case 3:
        rudder->channelChanged(raw);
        break;
    case 4:
        gyro->channelChanged(raw);
        break;
    case 5:
        pitch->channelChanged(raw);
        break;


    }
}

void RadioCalibrationWindow::saveFile()
{
    qDebug() << __FILE__ << __LINE__ << "SAVE TO FILE";
}

void RadioCalibrationWindow::loadFile()
{
    qDebug() << __FILE__ << __LINE__ << "LOAD FROM FILE";
}

void RadioCalibrationWindow::send()
{
    qDebug() << __FILE__ << __LINE__ << "uasId = " << uasId;
#ifdef MAVLINK_ENABLED_UALBERTA_MESSAGES
    UAS *uas = dynamic_cast<UAS*>(UASManager::instance()->getUASForId(uasId));
    if (uas)
    {
        qDebug()<< "we have a uas";
        mavlink_message_t msg;
        mavlink_msg_radio_calibration_pack(uasId, 0, &msg,
                                           (*radio)[RadioCalibrationData::AILERON],
                                           (*radio)[RadioCalibrationData::ELEVATOR],
                                           (*radio)[RadioCalibrationData::RUDDER],
                                           (*radio)[RadioCalibrationData::GYRO],
                                           (*radio)[RadioCalibrationData::PITCH],
                                           (*radio)[RadioCalibrationData::THROTTLE]);
        uas->sendMessage(msg);
    }
#endif
}

void RadioCalibrationWindow::request()
{
    qDebug() << __FILE__ << __LINE__ << "READ FROM UAV";
    UAS *uas = dynamic_cast<UAS*>(UASManager::instance()->getUASForId(uasId));
    if (uas)
    {
        mavlink_message_t msg;
        mavlink_msg_action_pack(uasId, 0, &msg, 0, 0, ::MAV_ACTION_CALIBRATE_RC);
        uas->sendMessage(msg);
    }
}

void RadioCalibrationWindow::receive(const QPointer<RadioCalibrationData>& radio)
{
    if (radio)
    {
        if (this->radio)
            delete this->radio;
        this->radio = new RadioCalibrationData(*radio);

        aileron->set((*radio)(RadioCalibrationData::AILERON));
        elevator->set((*radio)(RadioCalibrationData::ELEVATOR));
        rudder->set((*radio)(RadioCalibrationData::RUDDER));
        gyro->set((*radio)(RadioCalibrationData::GYRO));
        pitch->set((*radio)(RadioCalibrationData::PITCH));
        throttle->set((*radio)(RadioCalibrationData::THROTTLE));
    }        
}
