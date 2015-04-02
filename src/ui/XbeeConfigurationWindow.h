#ifndef _XBEECONFIGURATIONWINDOW_H_
#define _XBEECONFIGURATIONWINDOW_H_

#include <QObject>
#include <QWidget>
#include <QAction>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <LinkInterface.h>
#include"XbeeLinkInterface.h"
#include "HexSpinBox.h"


class XbeeConfigurationWindow : public QWidget
{
	Q_OBJECT

public:
	XbeeConfigurationWindow(LinkInterface* link, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
	~XbeeConfigurationWindow();

public slots:
	void configureCommunication();
	void setPortName(QString port);
	void setBaudRateString(QString baud);
	void setupPortList();

private slots:
	void addrChangedHigh(int addr);
	void addrChangedLow(int addr);

protected:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    bool userConfigured; ///< Switch to detect if current values are user-selected and shouldn't be overriden

private:
	QLabel *portLabel;
	QLabel *baudLabel;
	QComboBox *portBox;
	QComboBox *baudBox;
	QGridLayout *actionLayout;
	QHBoxLayout *xbeeLayout;
	QVBoxLayout *tmpLayout;
	XbeeLinkInterface* link;
    QTimer* portCheckTimer;
	HexSpinBox* highAddr;
	HexSpinBox* lowAddr;
	QLabel* highAddrLabel;
	QLabel* lowAddrLabel;

signals:
	void addrHighChanged(quint32 addrHigh);
	void addrLowChanged(quint32 addrLow);
};


#endif //_XBEECONFIGURATIONWINDOW_H_
