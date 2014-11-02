/* QespTest.cpp
**************************************/
#include "QespTest.h"
#include "qextserialport.h"
#include <QLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QSpinBox>


QespTest::QespTest(QWidget *parent)
    : QWidget(parent)

{
    //modify the port settings on your own
#ifdef Q_OS_UNIX
    port = new QextSerialPort(QLatin1String("/dev/ttyS0"), QextSerialPort::Polling);
#else
    port = new QextSerialPort(QLatin1String("COM1"), QextSerialPort::Polling);
#endif /*Q_OS_UNIX*/
    port->setBaudRate(BAUD19200);
    port->setFlowControl(FLOW_OFF);
    port->setParity(PAR_NONE);
    port->setDataBits(DATA_8);
    port->setStopBits(STOP_2);
    //set timeouts to 500 ms
    port->setTimeout(500);

    message = new QLineEdit(this);

    // transmit receive
    QPushButton *transmitButton = new QPushButton(tr("Transmit"));
    connect(transmitButton, SIGNAL(clicked()), SLOT(transmitMsg()));
    QPushButton *receiveButton = new QPushButton(tr("Receive"));
    connect(receiveButton, SIGNAL(clicked()), SLOT(receiveMsg()));
    QHBoxLayout *trLayout = new QHBoxLayout;
    trLayout->addWidget(transmitButton);
    trLayout->addWidget(receiveButton);

    //CR LF
    QPushButton *CRButton = new QPushButton(tr("CR"));
    connect(CRButton, SIGNAL(clicked()), SLOT(appendCR()));
    QPushButton *LFButton = new QPushButton(tr("LF"));
    connect(LFButton, SIGNAL(clicked()), SLOT(appendLF()));
    QHBoxLayout *crlfLayout = new QHBoxLayout;
    crlfLayout->addWidget(CRButton);
    crlfLayout->addWidget(LFButton);

    //open close
    QPushButton *openButton = new QPushButton(tr("Open"));
    connect(openButton, SIGNAL(clicked()), SLOT(openPort()));
    QPushButton *closeButton = new QPushButton(tr("Close"));
    connect(closeButton, SIGNAL(clicked()), SLOT(closePort()));
    QHBoxLayout *ocLayout = new QHBoxLayout;
    ocLayout->addWidget(openButton);
    ocLayout->addWidget(closeButton);

    received_msg = new QTextEdit();

    QVBoxLayout *myVBox = new QVBoxLayout;
    myVBox->addWidget(message);
    myVBox->addLayout(crlfLayout);
    myVBox->addLayout(trLayout);
    myVBox->addLayout(ocLayout);
    myVBox->addWidget(received_msg);
    setLayout(myVBox);

    qDebug("isOpen : %d", port->isOpen());
}

QespTest::~QespTest()
{
    delete port;
    port = NULL;
}

void QespTest::transmitMsg()
{
    int i = port->write(message->text().toLatin1());
    qDebug("trasmitted : %d", i);
}

void QespTest::receiveMsg()
{
    char buff[1024];
    int numBytes;

    numBytes = port->bytesAvailable();
    if(numBytes > 1024)
        numBytes = 1024;

    int i = port->read(buff, numBytes);
    if (i != -1)
        buff[i] = '\0';
    else
        buff[0] = '\0';
    QString msg = QLatin1String(buff);

    received_msg->append(msg);
    received_msg->ensureCursorVisible();
    qDebug("bytes available: %d", numBytes);
    qDebug("received: %d", i);
}


void QespTest::appendCR()
{
    message->insert(QLatin1String("\x0D"));
}

void QespTest::appendLF()
{
    message->insert(QLatin1String("\x0A"));
}

void QespTest::closePort()
{
    port->close();
    qDebug("is open: %d", port->isOpen());
}

void QespTest::openPort()
{
    port->open(QIODevice::ReadWrite | QIODevice::Unbuffered);
    qDebug("is open: %d", port->isOpen());
}

