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

    connect(aileron, SIGNAL(setpointChanged(int,uint16_t)), radio, SLOT(setAileron(int,uint16_t)));
    connect(elevator, SIGNAL(setpointChanged(int,uint16_t)), radio, SLOT(setElevator(int,uint16_t)));
    connect(rudder, SIGNAL(setpointChanged(int,uint16_t)), radio, SLOT(setRudder(int,uint16_t)));
    connect(gyro, SIGNAL(setpointChanged(int,uint16_t)), radio, SLOT(setGyro(int,uint16_t)));
    connect(pitch, SIGNAL(setpointChanged(int,uint16_t)), radio, SLOT(setPitch(int,uint16_t)));
    connect(throttle, SIGNAL(setpointChanged(int,uint16_t)), radio, SLOT(setThrottle(int,uint16_t)));
    setUASId(0);
}



void RadioCalibrationWindow::setChannel(int ch, float raw)
{
    /** this expects a particular channel to function mapping
       \todo allow run-time channel mapping
       */
    switch (ch) {
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
    QString fileName(QFileDialog::getSaveFileName(this,
                     tr("Save RC Calibration"),
                     "settings/",
                     tr("XML Files (*.xml)")));
    if (fileName.isEmpty())
        return;

    QDomDocument *rcConfig = new QDomDocument();

    QFile rcFile(fileName);
    if (rcFile.exists()) {
        rcFile.remove();
    }
    if (!rcFile.open(QFile::WriteOnly | QFile::Text)) {
        qDebug() << __FILE__ << __LINE__ << "could not open"  << rcFile.fileName() << "for writing";
        return;
    }

    QDomElement root;
    rcConfig->appendChild(root=rcConfig->createElement("channels"));
    QDomElement e;
    QDomText t;

    // Aileron
    e = rcConfig->createElement("threeSetpoint");
    e.setAttribute("name", "Aileron");
    e.setAttribute("number", "1");
    t = rcConfig->createTextNode(radio->toString(RadioCalibrationData::AILERON));
    e.appendChild(t);
    root.appendChild(e);
    // Elevator
    e = rcConfig->createElement("threeSetpoint");
    e.setAttribute("name", "Elevator");
    e.setAttribute("number", "2");
    t = rcConfig->createTextNode(radio->toString(RadioCalibrationData::ELEVATOR));
    e.appendChild(t);
    root.appendChild(e);
    // Rudder
    e = rcConfig->createElement("threeSetpoint");
    e.setAttribute("name", "Rudder");
    e.setAttribute("number", "4");
    t = rcConfig->createTextNode(radio->toString(RadioCalibrationData::RUDDER));
    e.appendChild(t);
    root.appendChild(e);
    // Gyro Mode/Gain
    e = rcConfig->createElement("twoSetpoint");
    e.setAttribute("name", "Gyro");
    e.setAttribute("number", "5");
    t = rcConfig->createTextNode(radio->toString(RadioCalibrationData::GYRO));
    e.appendChild(t);
    root.appendChild(e);
    // Throttle
    e = rcConfig->createElement("fiveSetpoint");
    e.setAttribute("name", "Throttle");
    e.setAttribute("number", "3");
    t = rcConfig->createTextNode(radio->toString(RadioCalibrationData::THROTTLE));
    e.appendChild(t);
    root.appendChild(e);
    // Pitch
    e = rcConfig->createElement("fiveSetpoint");
    e.setAttribute("name", "Pitch");
    e.setAttribute("number", "6");
    t = rcConfig->createTextNode(radio->toString(RadioCalibrationData::PITCH));
    e.appendChild(t);
    root.appendChild(e);


    QTextStream out(&rcFile);
    const int IndentSize = 4;
    rcConfig->save(out, IndentSize);
    rcFile.close();

}

void RadioCalibrationWindow::loadFile()
{
    QString fileName(QFileDialog::getOpenFileName(this,
                     tr("Load RC Calibration"),
                     "settings/",
                     tr("XML Files (*.xml)")));

    if (fileName.isEmpty())
        return;

    QFile rcFile(fileName);
    if (!rcFile.exists()) {
        return;
    }

    if (!rcFile.open(QIODevice::ReadOnly)) {
        return;
    }

    QDomDocument *rcConfig = new QDomDocument();

    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!rcConfig->setContent(&rcFile, true, &errorStr, &errorLine,
                              &errorColumn)) {
        qDebug() << "Error reading XML Parameter File on line: " << errorLine << errorStr;
        return;
    }

    rcFile.close();
    QDomElement root = rcConfig->documentElement();
    if (root.tagName() != "channels") {
        qDebug() << __FILE__ << __LINE__ << "This is not a Radio Calibration xml file";
        return;
    }


    QPointer<RadioCalibrationData> newRadio = new RadioCalibrationData();
    QDomElement child = root.firstChildElement();
    while (!child.isNull()) {
        parseSetpoint(child, newRadio);
        child = child.nextSiblingElement();
    }

    receive(newRadio);

    delete newRadio;
    delete rcConfig;
}

void RadioCalibrationWindow::parseSetpoint(const QDomElement &setpoint, const QPointer<RadioCalibrationData>& newRadio)
{
    QVector<float> setpoints;
    QStringList setpointList = setpoint.text().split(",", QString::SkipEmptyParts);
    foreach (QString setpoint, setpointList)
    setpoints << setpoint.trimmed().toFloat();

//    qDebug() << __FILE__ << __LINE__ << ": " << setpoint.tagName() << ": " << setpoint.attribute("name") ;
    if (setpoint.tagName() == "threeSetpoint") {
        if (setpoints.isEmpty())
            setpoints << 0 << 0 << 0;
        for (int i=0; i<3; ++i) {
            if (setpoint.attribute("name").toUpper() == "AILERON")
                newRadio->setAileron(i, setpoints[i]);
            else if(setpoint.attribute("name").toUpper() == "ELEVATOR")
                newRadio->setElevator(i, setpoints[i]);
            else if(setpoint.attribute("name").toUpper() == "RUDDER")
                newRadio->setRudder(i, setpoints[i]);
        }
    } else if (setpoint.tagName() == "twoSetpoint") {
        if (setpoints.isEmpty())
            setpoints << 0 << 0;
        for (int i=0; i<2; ++i) {
            if (setpoint.attribute("name").toUpper() == "GYRO")
                newRadio->setGyro(i, setpoints[i]);
        }
    } else if (setpoint.tagName() == "fiveSetpoint") {
        if (setpoints.isEmpty())
            setpoints << 0 << 0 << 0 << 0 << 0;
        for (int i=0; i<5; ++i) {
            if (setpoint.attribute("name").toUpper() == "PITCH")
                newRadio->setPitch(i, setpoints[i]);
            else if (setpoint.attribute("name").toUpper() == "THROTTLE")
                newRadio->setThrottle(i, setpoints[i]);
        }
    }
}

void RadioCalibrationWindow::send()
{
    qDebug() << __FILE__ << __LINE__ << "uasId = " << uasId;
#ifdef MAVLINK_ENABLED_UALBERTA
    UAS *uas = dynamic_cast<UAS*>(UASManager::instance()->getUASForId(uasId));
    if (uas) {
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
    if (uas) {
        mavlink_message_t msg;
        mavlink_msg_action_pack(uasId, 0, &msg, 0, 0, ::MAV_ACTION_CALIBRATE_RC);
        uas->sendMessage(msg);
    }
}

void RadioCalibrationWindow::receive(const QPointer<RadioCalibrationData>& radio)
{
    if (radio) {
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
